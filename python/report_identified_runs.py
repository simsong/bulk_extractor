#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#
# It works with the improved identified_blocks_explained available in hashdb 1.1


import sqlite3
import os,pickle,json,sys,csv
from collections import defaultdict
from be_image_reader import BEImageReader,all_null

UTF8_BOM = "\ufeff"

if sys.version_info < (3,0,0):
    raise RuntimeError("Requires Python 3.0 or above")

def get_filesize(fn):
    """Attempt to determine the filesize of file fn"""
    if not fn: return 0
    if os.path.exists(fn):
        return os.path.getsize(fn)
    return 0

DELIM=u'\U0010001c'

def clean_target_filename(target_filename):
    if DELIM in target_filename:
        target_filename=target_filename.replace(DELIM,"")
    return target_filename


#
# prepare a report of the directory 'dn' examining the
# identified_blocks.txt file and the identified_blocks_expanded.txt
# files.
#
# Each block has a disk offset, a file offset, a hash, a file it was seen in.

hash_count              = dict() # how many times we have seen this hash
hash_flags              = dict() # flags for this hash
hash_source_file_blocks = dict() # for each hash, a dict of sets of the offsets were it was found
source_id_filenames     = dict() # filename for each source id
hash_disk_blocks        = defaultdict(set) # all of the disk blocks where a given hash was seen
hash_flags              = dict() # flags for a given hash
hashes_for_source       = defaultdict(set) # for each source, a set of its hashes found
source_id_count         = defaultdict(int)
source_id_filesizes     = dict()
candidate_sources       = set()

def read_explained_file(reportdir):
    if reportdir.endswith("/") or reportdir.endswith("\\"):
        reportdir = reportdir[:-1]

    # Read the identified_blocks_explained file twice.
    # The first time is for candidate selection. The second time is to build the database
    identified_blocks_fn = os.path.join(reportdir,"identified_blocks_explained.txt")

    # Candidate Selection
    print("reading {} for candidate selection".format(identified_blocks_fn))
    for line in open(identified_blocks_fn):
        if line[0]=='[':        # a hash and its sources
            (hash,meta,sources) = json.loads(line)
            count = meta.get('count',0)
            flags = meta.get('flags',None)
            if (count>=1 and not flags) or args.all:
                for s in sources:
                    candidate_sources.add(s['source_id'])

    # Now re-read the file, but only consider blocks for candidates

    print("re-reading {} to build source database".format(identified_blocks_fn))
    for line in open(identified_blocks_fn):
        if line[0]=='[':        # a hash and its sources
            (hash,meta,sources) = json.loads(line)
            is_candidate = False
            for s in sources:
                if s['source_id'] in candidate_sources:
                    is_candidate = True
                    break
            if not is_candidate: continue

            # This is a hash from a candidate. Add it to the memory database
            hash_count[hash] = count = meta.get('count',0)
            hash_flags[hash] = flags = meta.get('flags',None)
            assert(hash not in hash_source_file_blocks)
            hash_source_file_blocks[hash] = defaultdict(set)

            for s in sources:
                source_id  = s['source_id']
                if source_id in candidate_sources:
                    file_block = s['file_offset'] // 4096
                    hashes_for_source[source_id].add(hash)
                    hash_source_file_blocks[hash][source_id].add(file_block)
                    source_id_count[source_id] += 1

        # Learn about the sources 
        if line[0]=='{':
            d = json.loads(line)
            source_id = d['source_id']
            filename = clean_target_filename(d['filename'])
            source_id_filenames[source_id] = filename
            source_id_filesizes[source_id]  = d.get('filesize',get_filesize(filename))
        
def get_disk_offsets(reportdir):
    identified_blocks_fn = os.path.join(reportdir,"identified_blocks.txt")
    print("reading "+identified_blocks_fn)
    for line in open(identified_blocks_fn):
        try:
            (disk_offset,sector_hash,meta) = line.split("\t")
            disk_offset = int(disk_offset)
        except ValueError:
            continue
        hash_disk_blocks[sector_hash].add(disk_offset // 512)

def hash_sets(reportdir):
    # Implements the HASH-SETS algorithm. Basically reports which files are most likely to be present
    # dump the database, I guess
    # Make a list of the all the sources and their score
    
    print("running hash_sets")
    import math

    report_fn = os.path.join(reportdir,"hash-sets-report.csv")
    print("Writing HASH-SETS output to {}".format(report_fn))
    of = open(report_fn, 'w', newline='')
    ofwriter = csv.writer(of,  dialect='excel')

    ofwriter.writerow(['Filename','Raw Score','Normalized Score'])
    res = []
    for source_id in candidate_sources:
        norm = ''
        if source_id in source_id_filesizes:
            norm = source_id_count[source_id] / (math.floor(source_id_filesizes[source_id]/4096))
            if norm>1: norm=1   # for cases where we successfully hashed the last block
        res.append([source_id_filenames[source_id],source_id_count[source_id],norm])
    res.sort(reverse=True,key=lambda a:(a[2],a[1]))
    ofwriter.writerows(res)


def exists_a_larger(set1,set2):
    # Return true if there exists an element in set2 that is one larger
    # than an element in set1
    for s1 in set1:
        if s1+1 in set2: return True
    return False

def hash_runs(reportdir):
    # Is there a sqlite3 database?

    print("Running hash_runs")
    conn = None
    cur  = None
    dbname = os.path.join(reportdir,args.dbname)
    if os.path.exists(dbname):
        print("Reading {}".format(dbname))
        import sqlite3
        conn = sqlite3.connect("file:{}?mode=ro".format(dbname),uri=True)
        cur  = conn.cursor()
        cur.execute("PRAGMA cache_size = -1000000") # a gigabyte of cache
    else:
        print("Will not report allocated files; no file {}".format(dbname))

    def get_filename(block):
        if not cur: return (None,None)
        byte_start = block * 512
        #
        # This is wrong becuase it doesn't take into account img_offset (byte_start is from the start of the partition, not the start of the image)
        # cur.execute("SELECT parent_path||name,size from tsk_files where obj_id in (SELECT obj_id from tsk_file_layout where byte_start<=? and byte_start+byte_len>?)", (byte_start,byte_start))

        cur.execute("select B.parent_path||B.name,size from tsk_file_layout as A JOIN tsk_files as B on A.obj_id=B.obj_id JOIN tsk_fs_info as C on B.fs_obj_id=C.obj_id where byte_start+img_offset<=? and byte_start+img_offset+byte_len>?",(byte_start,byte_start))

        r = cur.fetchone()
        if r:
            (name,size) = r
            if not name:
                size = None
            return (name,size)
        return (None,None)

    # Now generate the output
    # Read the v1.1 identified_blocks file and build the list of
    # where each hashes associated with each file.

    # Open the report file
    report_fn = os.path.join(reportdir,"hash-runs-report.csv")
    print("writing "+report_fn)
    of = open(report_fn, 'w', encoding='utf-8', newline='')
    of.write(UTF8_BOM)          # makes Excel open the file as Unicode
    ofwriter = csv.writer(of,  dialect='excel')

    # Now, for every source, make an array of all the blocks that were found

    if args.debug: print("Total Candidates:",len(candidate_sources))
    total_combined_rows = 0

    for source_id in sorted(candidate_sources,
                            key=lambda id:(-source_id_count[id],source_id_count[id])):
        filename = source_id_filenames[source_id]
        if args.debug: print("candidate source_id",source_id,"filename",filename)

        mod8_block_runs = []
        for i in range(0,8):
            mod8_block_runs.append([])

        # Build the block_runs array for each mod8 value
        # Each element in the array inconsists of:
        # [0] = disk_block (512-byte sectors)
        # [1] = set of file_blocks (4096-byte blocks)
        # [2] = count of blocks
        for hash in hashes_for_source[source_id]:
            for disk_block in hash_disk_blocks[hash]:
                mod8 = disk_block % 8
                file_blocks = set()
                for file_block in hash_source_file_blocks[hash][source_id]:
                    file_blocks.add(file_block)
                mod8_block_runs[mod8].append((disk_block,file_blocks,hash_count[hash]))

        # now loop for each mod8 value
        rows = []
        for block_runs in mod8_block_runs:
            if not block_runs: continue # empty
            block_runs.sort()
            run_start = 0
            while run_start < len(block_runs):
                # advance run_end until it points to the end of the run
                run_end   = run_start
                while ((run_end+1 < len(block_runs) and
                        (block_runs[run_end][0]+8 == block_runs[run_end+1][0]) and
                        (exists_a_larger(block_runs[run_end][1],block_runs[run_end+1][1])))):
                        run_end += 1

                # We are at the end of a run
                # For debugging at the moment, print the whole thing out
                if args.debug:
                    for r in range(run_start,min(run_end+1,len(block_runs))):
                        print(r,block_runs[r])
                    print("")

                if of.tell()<4:
                    ofwriter.writerow(['Identified File','Score','Physical Block Start',
                                       'Logical Block Start','Logical Block End',
                                       'File Blocks','(mod 8)','Percentage',
                                       'Source File','Source Size'])
                counts = [br[2] for br in block_runs[run_start:run_end+1]]

                if min(counts) > args.mincount:
                    # all of the counts are too high!
                    run_start = run_end+1
                    continue

                score  = sum(map(lambda inv:1.0/inv,counts))
                file_blocks = source_id_filesizes[source_id]//4096
                physical_block_start = block_runs[run_start][0]
                logical_block_start = min(block_runs[run_start][1])
                logical_block_end = min(block_runs[run_end][1])
                rows.append([filename,score,physical_block_start,
                             logical_block_start,logical_block_end,
                             file_blocks,physical_block_start % 8 ])
                run_start = run_end+1
        # sort the rows by starting logical block
        rows.sort(key=lambda a:a[4])

        # Are there runs that can be combined because the blocks on the disk are blank?
        def test_combine_rows(a,b):
            if args.debug: print("can_combine({},{})".format(a,b))
            if(rows[a][0]!=rows[b][0]):return 0
            physical_block_start0 = rows[a][2]
            logical_block_start0 = rows[a][3]
            logical_block_end0   = rows[a][4]

            physical_block_start1 = rows[b][2]
            logical_block_start1 = rows[b][3]
            if args.debug: print(physical_block_start1-physical_block_start0,logical_block_start1-logical_block_start0)
            if (physical_block_start1-physical_block_start0) != (logical_block_start1-logical_block_start0)*8:
                return 0
            if args.debug: print("rows {} and {} might be combined".format(i,i+1))
            # If the blocks from physical_block_start0+1 through physical_block_start1-1 are all null, then they can!
            combine = True
            br = BEImageReader(args.image)
            # Note: we need to add 2 below. 1 to get the length of the run and another to step into the next region
            physical_block_end0 = physical_block_start0+(logical_block_end0+2-logical_block_start0)*8
            null_blocks = 0
            for sector in range(physical_block_end0,physical_block_start1,8):
                buf = br.read(sector*512,4096)
                if args.debug: print("check",sector,"=",all_null(buf))
                if not all_null(buf):
                    combine = False
                    return 0
                null_blocks += 1
            if args.debug: print("CAN be combined")
            return null_blocks

        # See if runs can be combined
        for i in range(len(rows)-1,0,-1):
            null_blocks = test_combine_rows(i-1,i)
            if null_blocks > 0:
                if args.debug:
                    print("combine rows {} and {}".format(i-1,i))
                    print("old: {} {}".format(rows[i-1],rows[i]))
                rows[i-1][1] += rows[i][1] + null_blocks # increment score
                rows[i-1][4] = rows[i][4]  # logical_block_end
                if args.debug:
                    print("new: {}\n".format(rows[i-1]))
                del rows[i]                # and combine the rows
                total_combined_rows += 1
                
        # Delete the runs that are two small (after the combine step)
        for i in range(len(rows)-1,-1,-1):
            if rows[i][4] - rows[i][3] + 1 <= args.minrun:
                del rows[i]

        # Finally determine the source files and compute the percentages (after rows are combined)
        for row in rows:
            (source_file,source_size) = get_filename(row[2])
            file_blocks = row[6]
            found_blocks = row[4] - row[3] + 1
            if found_blocks==file_blocks+1: file_blocks +=1 # sometimes we get the partial last block
            percentage = "{:3.0f}%".format(found_blocks / file_blocks * 100.0)
            row += [percentage,source_file,source_size]

        # Now write the rows
        for row in rows:
            ofwriter.writerow(row)
    print("Rows combined: {}".format(total_combined_rows))
    


if __name__=="__main__":
    import sys
    import argparse
    from subprocess import call
    parser = argparse.ArgumentParser()
    
    parser.add_argument("reportdir",help="Report directory")
    parser.add_argument("--all",action='store_true',help='show all files; disable candidate selection')
    parser.add_argument("--debug",action='store_true',help='print debug info')
    parser.add_argument("--minrun",help='do not report runs shorter than this',default=2)
    parser.add_argument("--mincount",help='at least one of the blocks in each run must have a count less than this',default=10)
    parser.add_argument("--run",help="run tsk_loaddb if not hasn't been run",action='store_true')
    parser.add_argument("--dbname",help="name of tsk_loaddb database to use in REPORTDIR",default='tsk_db.sqlite3')
    parser.add_argument("--explain",help="If identified_blocks_explained.txt is not present, run hashdb to produce it, and use this database")
    parser.add_argument("--image",help="image file (defaults to what is in BE report)")
    args = parser.parse_args()

    if not os.path.exists(args.reportdir):
        print("{} does not exist".format(args.reportdir))
        exit(1)

    explainfile = os.path.join(args.reportdir,"identified_blocks_explained.txt")
    if not os.path.exists(explainfile) and args.explain:
        ifname = os.path.join(args.reportdir,"identified_blocks.txt")
        ofname = os.path.join(args.reportdir,"identified_blocks_explained.txt")
        cmd = ['hashdb','explain_identified_blocks',args.explain,ifname]
        print(" ".join(cmd))
        call(cmd,stdout=open(ofname,"w"))


    if not args.image:
        from bulk_extractor_reader import BulkReport
        b = BulkReport(args.reportdir)
        args.image = b.image_filename()

    dbname = os.path.join(args.reportdir,args.dbname)
    if not os.path.exists(dbname) and args.run:
        print("{} does not exist. Will try to run tsk_loaddb".format(dbname))
        cmd=['tsk_loaddb','-d',dbname,args.image]
        print(" ".join(cmd))
        call(cmd)
        # Add the indexes
        print("Adding indexes to database")
        import sqlite3
        con = sqlite3.connect("file:{}".format(dbname),uri=True)
        cur  = con.cursor()
        cur.execute("create index if not exists start1 on tsk_file_layout(byte_start)");
        cur.execute("create index if not exists start2 on tsk_file_layout(byte_len)");
        con.close()



    read_explained_file(args.reportdir)
    hash_sets(args.reportdir)
    get_disk_offsets(args.reportdir)
    hash_runs(args.reportdir)
