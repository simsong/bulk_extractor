#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database.
# It is for the the hashdb version 1.0 "identified_blocks" file.


import os,pickle
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

    hash_counts            = defaultdict(int) # number of times each hash seen
    identified_file_blocks = defaultdict(list) # blocks seen for each file
    identified_file_score  = defaultdict(float) # score for each file

    # Read the identified_blocks file to find out how many times
    # each hash was found.

    identified_blocks_fn = os.path.join(reportdir,"identified_blocks.txt")
    print("reading "+identified_blocks_fn)
    for line in open(identified_blocks_fn):
        try:
            (disk_offset,sector_hash,count) = line.split("\t")
        except ValueError:
            continue

        # For now, ignore the annotations
        if 'H' in count:
            count = count.replace('H','')
        if 'W' in count:
            count = count.replace('W','')
        if 'R' in count:
            count = count.replace('R','')
        count = count.replace(' ','')
        hash_counts[sector_hash] = int(count)

    # This is for reporting the mod8 offsets
    identified_file_mod8set = defaultdict(dict)


    # 
    # Read the identified_blocks_expanded file to determine, for each file,
    # the blocks that we've found and the sector hashes associated with that file.
    
    identified_blocks_expanded_fn = os.path.join(reportdir,"identified_blocks_expanded.txt")
    print("reading "+identified_blocks_expanded_fn)
    for line in open(identified_blocks_expanded_fn):
        (disk_offset,sector_hash,meta) = line.split("\t")
        disk_offset = int(disk_offset)
        disk_block  = disk_offset // 512
        filename    = meta.split(",")[1].split('=')[1]
        file_offset = int(meta.split(",")[2].split('=')[1])
        file_block  = file_offset // 4096
        identified_file_blocks[filename].append((disk_block,file_block,hash_counts[sector_hash]))
        identified_file_score[filename] += 1/hash_counts[sector_hash] # just for sorting

    # Open the report file

    report_fn = os.path.join(reportdir,"blocks-report.csv")
    print("writing "+report_fn)

    if sys.version_info >= (3,0,0):
        of = open(report_fn, 'w', newline='')
    else:
        of = open(report_fn, 'wb')
    import csv
    ofwriter = csv.writer(of,  dialect='excel')

    # Inverse Sort by the score
    filenames_sorted = sorted(identified_file_blocks.keys(),key=lambda fn:identified_file_score[fn],reverse=True)

    # Print the report

    first = True
    for filename in filenames_sorted:
        clean_filename = clean_target_filename(filename)
        identified_file_blocks[filename].sort()
        for mod8 in range(0,8):
            block_runs = list(filter(lambda a:(a[0] % 8) == mod8,identified_file_blocks[filename] ))
            if block_runs:
                run = []
                has_singleton = False
                while block_runs:
                    # See if the run can be extended
                    if (run==[]) or (run[-1][0]+8==block_runs[0][0] and run[-1][1]+1==block_runs[0][1]):
                        if block_runs[0][2]==1: has_singleton = True
                        run.append(block_runs[0])
                        block_runs = block_runs[1:]
                        # loop if there are more that could be in the run
                        if block_runs: 
                            continue
                    if has_singleton and len(run)>1:
                        print("{} mod8={} ".format(clean_filename,mod8))
                        if len(run)<100:
                            for br in run:
                                print(br)
                            print("")
                        counts = [br[2] for br in run]
                        score  = sum(map(lambda inv:1.0/inv,counts))
                        if first:
                            ofwriter.writerow(['Identified File','Score','Physical Block Start','Logical Block Start','--','Logical Block End'])
                        first = False
                        ofwriter.writerow([clean_filename,score,run[0][0],run[0][1],'--',run[-1][1]])
                    run = []
                    has_singleton = False
                        
        # print(clean_filename,identified_file_mod8set[filename])
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
