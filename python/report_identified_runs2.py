#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#
# It works with the improved identified_blocks_explained available in hashdb 1.1


import sqlite3
import os,pickle,json
from collections import defaultdict

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
hash_disk_blocks        = defaultdict(set)
hash_flags              = dict() # flags for a given hash
hashes_for_source       = defaultdict(set) # for each source, a set of its hashes found
source_id_scores        = defaultdict(int)
source_id_flags         = defaultdict(str)             # flags for (source_id, file_block)
source_id_filesize      = dict()
candidate_sources       = set()


def read_explained_file(reportdir):
    if reportdir.endswith("/") or reportdir.endswith("\\"):
        reportdir = reportdir[:-1]

    # Read the identified_blocks_explained file first
    identified_blocks_fn = os.path.join(reportdir,"identified_blocks_explained.txt")
    print("reading "+identified_blocks_fn)
    for line in open(identified_blocks_fn):
        if line[0]=='[':        # a hash and its sources
            (hash,meta,sources) = json.loads(line)
            hash_count[hash] = count = meta.get('count',0)
            hash_flags[hash] = flags = meta.get('flags',None)
            hash_source_file_blocks[hash] = defaultdict(set)

            for s in sources:
                source_id  = s['source_id']
                file_block = s['file_offset'] // 4096
                source_id_scores[source_id] += 1/count # for sorting
                hashes_for_source[source_id].add(hash)
                hash_source_file_blocks[hash][source_id].add(file_block)
                if meta and 'flags' in meta:
                    source_id_flags[(source_id,file_block)] = meta['flags']
                if count==1 and not flags:
                    candidate_sources.add(source_id)

        # Learn about the sources 
        if line[0]=='{':
            d = json.loads(line)
            source_id = d['source_id']
            filename = clean_target_filename(d['filename'])
            source_id_filenames[source_id] = filename
            source_id_filesize[source_id]  = d.get('filesize',get_filesize(filename))
        

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

import csv
def hash_sets(reportdir,report_fn):
    # dump the database, I guess
    # Make a list of the all the sources and their score
    
    of = open(report_fn, 'w', newline='')
    ofwriter = csv.writer(of,dialect='excel')

    ofwriter.writerow(['Filename','Score'])
    for source_id in sorted(candidate_sources,key=lambda id:source_id_scores[id],reverse=True):
        ofwriter.writerow([source_id_filenames[source_id],source_id_scores[source_id]])


def hash_runs(reportdir):
    # Is there a sqlite3 database?

    conn = None
    cur  = None
    dbname = os.path.join(reportdir,"tsk_db.sqlite3")
    if os.path.exists(dbname):
        import sqlite3
        conn = sqlite3.connect(dbname)
        cur  = conn.cursor()

    # Read the v1.1 identified_blocks file and build the list of
    # where each hashes associated with each file.



    # Open the report file
    report_fn = os.path.join(reportdir,"blocks-report.csv")
    print("writing "+report_fn)

    if sys.version_info >= (3,0,0):
        of = open(report_fn, 'w', newline='')
    else:
        of = open(report_fn, 'wb')
    import csv
    ofwriter = csv.writer(of,  dialect='excel')

    # Now, for every source, make an array of all the blocks that were found

    first = True
    for source_id in sorted(source_id_scores.keys(),key=lambda id:source_id_scores[id],reverse=True):
        filename = source_id_filenames[source_id]
        identified_file_blocks = []
        for hash in hashes_for_source[source_id]:
            for disk_block in hash_disk_blocks[hash]:
                file_blocks = set()
                for file_block in hash_source_file_blocks[hash][source_id]:
                    file_blocks.add(file_block)
                identified_file_blocks.append((disk_block,file_blocks,len(hash_source_file_blocks[hash])))


        # Now generate the output
        identified_file_blocks.sort()

        # Each element in the array inconsists of:
        # [0] = disk_block (512-byte sectors)
        # [1] = set of file_blocks (4096-byte blocks)
        # [2] = count (how many sources we've seen it in.)

        def exists_a_larger(set1,set2):
            # Return true if there exists an element in set2 that is one larger than an element in set1
            for s1 in set1:
                for s2 in set2:
                    if s1+1==s2: return True
            return False

        def get_filename(block):
            if not cur: return ("",-1)
            byte_start = block * 512
            cur.execute("SELECT obj_id from tsk_file_layout where byte_start<=? and byte_start+byte_len>?",
                        (byte_start,byte_start))
            r = cur.fetchone()
            if not r: return ("",-1)
            cur.execute("SELECT parent_path||name,size from tsk_files where obj_id=?",(r[0],))
            r2 = cur.fetchone()
            if not r2: return ("",-1)
            return (r2[0],r2[1])
            

        for mod8 in range(0,8):
            block_runs = list(filter(lambda a:(a[0] % 8) == mod8,identified_file_blocks))
            if not block_runs: continue
            run = []   
            has_singleton = False
            while block_runs:
                # See if the run can be extended
                if (run==[]) or (run[-1][0]+8==block_runs[0][0] and exists_a_larger(run[-1][1],block_runs[0][1])):
                    if block_runs[0][2]==1: has_singleton = True
                    run.append(block_runs[0])
                    block_runs = block_runs[1:]
                    # loop if there are more that could be in the run
                    if block_runs: 
                        continue
                if has_singleton and len(run)>1:
                    # For debugging, print runs that are less than 100 lines
                    print("{} mod8={} using {} of {} blocks of {} byte file ({:6.2f}%) {} ".
                          format(filename,mod8,len(run),len(identified_file_blocks),
                                 source_id_filesize[source_id],len(run)*4096/source_id_filesize[source_id]*100.0,source_id
                             ))
                    no_flags = True
                    if len(run)<100:
                        no_flags = False
                        for br in run:
                            flags = source_id_flags[(source_id,min(br[1]))]
                            print(br,flags)
                            if not flags: no_flags=True
                        if not no_flags:
                            print("will ignore; all entries have flags set")
                        print("")
                    if no_flags:
                        counts = [br[2] for br in run]
                        score  = sum(map(lambda inv:1.0/inv,counts))
                        if first:
                            ofwriter.writerow(['Identified File','Score','Physical Block Start',
                                               'Logical Block Start','--','Logical Block End','(mod 8)',
                                               'Source File','Source Size'])
                        first = False
                        (source_file,source_size) = get_filename(run[0][0])
                        ofwriter.writerow([filename,score,run[0][0],
                                           run[0][1],'--',run[-1][1],mod8,
                                           source_file,source_size ])
                run = []
                has_singleton = False


if __name__=="__main__":
    import sys
    import argparse
    parser = argparse.ArgumentParser()
    
    parser.add_argument("reportdir",help="Report directory")
    parser.add_argument("--sets",help='run HASH-SETS algorithm, output to SETS')
    args = parser.parse_args()

    if not os.path.exists(args.reportdir):
        print("{} does not exist".format(args.reportdir))
        exit(1)

    read_explained_file(args.reportdir)
    if args.sets:
        hash_sets(args.reportdir,args.sets)
    else:
        get_disk_offsets(args.reportdir)
        hash_runs(args.reportdir)
