#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#
# It works with improved identified_blocks_explained available in hashdb 1.1


import os,pickle,json
from collections import defaultdict

def clean_target_filename(target_filename):
    if sys.version_info >= (3,0,0):
        target_filename=target_filename.replace(u'\U0010001c',"")
    else:
        target_filename=target_filename.replace(b"\xe2\xa0\xc3\xc3\xa3","")
    return target_filename

#
# prepare a report of the directory 'dn' examining the
# identified_blocks.txt file and the identified_blocks_expanded.txt
# files.
#
# Each block has a disk offset, a file offset, a hash, a file it was seen in.

def process_report(reportdir):
    if reportdir.endswith("/") or reportdir.endswith("\\"):
        reportdir = reportdir[:-1]

    source_id_filenames     = dict()
    hash_disk_blocks        = defaultdict(set)
    hash_flags              = dict()
    hashes_for_source       = defaultdict(set)
    hash_source_file_blocks = dict()             # for each hash, a dict of lists of the offsets were it was found
    source_id_scores        = defaultdict(int)

    # Read the identified_blocks_explained file first
    identified_blocks_fn = os.path.join(reportdir,"identified_blocks_explained.txt")
    print("reading "+identified_blocks_fn)
    for line in open(identified_blocks_fn):
        if line[0]=='[':        # a hash and its sources
            (hash,flags,sources) = json.loads(line)
            hash_source_file_blocks[hash] = defaultdict(set)
            count = len(sources)
            for s in sources:
                source_id = s['source_id']
                file_block = s['file_offset'] // 4096
                source_id_scores[source_id] += 1/count # for sorting
                hashes_for_source[source_id].add(hash)
                hash_source_file_blocks[hash][source_id].add(file_block)
        if line[0]=='{':
            d = json.loads(line)
            source_id_filenames[d['source_id']] = clean_target_filename(d['filename'])
        

    # Read the v1.1 identified_blocks file and build the list of where each hashes associated with each file.

    identified_blocks_fn = os.path.join(reportdir,"identified_blocks.txt")
    print("reading "+identified_blocks_fn)
    for line in open(identified_blocks_fn):
        try:
            (disk_offset,sector_hash,meta) = line.split("\t")
            disk_offset = int(disk_offset)
        except ValueError:
            continue
        hash_disk_blocks[sector_hash].add(disk_offset // 512)


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

        # [0] = disk_block
        # [1] = set of file_blocks
        # [2] = count
        # [3] = flags (not currently used)

        def exists_a_larger(set1,set2):
            # Return true if there exists an element in set2 that is one larger than an element in set1
            for s1 in set1:
                for s2 in set2:
                    if s1+1==s2: return True
            return False

        for mod8 in range(0,8):
            block_runs = list(filter(lambda a:(a[0] % 8) == mod8,identified_file_blocks))
            if block_runs:
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
                        print("{} mod8={} using {} of {}".format(filename,mod8,len(block_runs),len(identified_file_blocks)))
                        if len(run)<100:
                            for br in run:
                                print(br)
                            print("")
                        counts = [br[2] for br in run]
                        score  = sum(map(lambda inv:1.0/inv,counts))
                        if first:
                            ofwriter.writerow(['Identified File','Score','Physical Block Start',
                                               'Logical Block Start','--','Logical Block End'])
                        first = False
                        ofwriter.writerow([filename,score,run[0][0],
                                           run[0][1],'--',run[-1][1]])
                    run = []
                    has_singleton = False


if __name__=="__main__":
    import sys
    import argparse
    parser = argparse.ArgumentParser()
    
    parser.add_argument("reportdir",help="Report directory")
    args = parser.parse_args()

    if not os.path.exists(args.reportdir):
        print("{} does not exist".format(args.reportdir))
        exit(1)

    process_report(args.reportdir)
