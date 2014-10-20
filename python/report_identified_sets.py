#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#


import os,pickle
from collections import defaultdict

def make_runs(a):
    a = sorted(list(a))
    ret = []
    first = None
    while len(a):
       if not first:
           first = a.pop(0)
           last  = first
           if not a:
               ret.append((first,last))
           continue
       if last+1 == a[0]:
           last = a.pop(0)
           if not a:
               ret.append((first,last))
           continue
       ret.append((first,last))
       first = None
    return ret


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
    identified_file_blocks = defaultdict(set) # blocks seen for each file
    identified_file_score  = defaultdict(float) # score for each file

    file_offsets = defaultdict(list)
    disk_offsets_for_hash = defaultdict(set) # all of the places where a specific has was seen

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
        filename    = meta.split(",")[1].split('=')[1]
        file_offset = int(meta.split(",")[2].split('=')[1])
        file_block  = file_offset // 4096
        identified_file_blocks[filename].add(file_block)
        identified_file_score[filename] += 1/hash_counts[sector_hash]

        # This is just for the mod8 offsets
        mod8 = (disk_offset / 512) % 8
        try:
            identified_file_mod8set[filename][mod8] += 1
        except KeyError:
            identified_file_mod8set[filename][mod8] = 1
    

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
        blocks = identified_file_blocks[filename]
        clean_filename = clean_target_filename(filename)
        score = identified_file_score[filename]
        b1  = min(blocks) # first block seen
        b2  = max(blocks) # last block seen
        block_range      = "{}--{}".format(b1,b2)
        count = len(blocks) # number of blocks seen
        missing = (b2-b1+1)-count         # missing between b1 and b2

        runs = make_runs(blocks)
        lens = [a[1]-a[0]+1 for a in runs]
        number_runs    = len(runs)
        longest_run    = max(lens)

        pmatch         = filename
        blocks_seen    = count
        
        if sys.version_info >= (3,0,0):
            pmatch=pmatch.replace(u'\U0010001c',"")
        else:
            pmatch=pmatch.replace(b"\xe2\xa0\xc3\xc3\xa3","")
        if first:
            ofwriter.writerow(['Identified File','Score','Blocks Seen','Block Range','# runs','Longest run'])
            first = False
        ofwriter.writerow([clean_filename,score,blocks_seen,block_range,
                           number_runs,
                           longest_run])
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
