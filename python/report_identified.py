#
# This program reads one or more databases created by the identified_blocks search
# and builds an in-memory database. It then supports queries on that database
#


import os,pickle
import collections

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

def process_explained(fname,dbfile):
    blocks_for_filename = collections.defaultdict(set)
    hashes_for_filename = collections.defaultdict(set)
    files_for_hash = collections.defaultdict(set)
    for line in open(fname):
        if line[0]=='#': continue
        (hash,repo,filename,offset) = line.split(", ")[0:4]
        block_number = int(offset) // 4096
        blocks_for_filename[filename].add(block_number)
        hashes_for_filename[filename].add(hash)
        files_for_hash[hash].add(filename)
    
    for filename in blocks_for_filename:
        runs = make_runs(blocks_for_filename[filename])
        hash_counts = [len(files_for_hash[a]) for a in hashes_for_filename[filename]]
        number_distinct_blocks = sum(filter(lambda a:a==1,hash_counts))
        if number_distinct_blocks==0: continue
        print("filename: {}".format(filename))
        print("  runs: ",runs)
        print("  lengths: ",[a[1]-a[0]+1 for a in runs])
        print("  distinct blocks: ",number_distinct_blocks)


#
# prepare a report of the directory 'dn' examining the
# identified_blocks.txt file and the identified_blocks_expanded.txt
# files.
#
# Each block has a disk offset, a file offset, a hash, a file it was seen in.


def process_expanded(dn,dbfile):
    if os.path.exists(dbfile):
        print("{} already exists; will just produce report.".format(dbfile))
        return

    hash_counts = collections.defaultdict(int) # number of times each hash seen
    file_blocks = collections.defaultdict(set) # blocks seen for each file
    distinct_file_blocks = collections.defaultdict(set) # blocks seen for each file
    file_sector_hashes = collections.defaultdict(set)
    file_offets = collections.defaultdict(list)
    disk_offsets_for_hash = collections.defaultdict(set) # all of the places where a specific has was seen
    h_hashes = set()
    w_hashes = set()
    r_hashes = set()

    # Read the identified_blocks file to find out how many times
    # each hash was found.

    hashes = dict()

    for line in open(os.path.join(dn,"identified_blocks.txt")):
        try:
            (disk_offset,sector_hash,count) = line.split("\t")
        except ValueError:
            continue
        if 'H' in count:
            count = count.replace(' H','')
            h_hashes.add(sector_hash)
        if 'W' in count:
            count = count.replace(' W','')
            w_hashes.add(sector_hash)
        if 'R' in count:
            count = count.replace(' R','')
            r_hashes.add(sector_hash)
        hash_counts[sector_hash] = int(count)

    # 
    # Read the identified_blocks_expanded file to determine, for each file,
    # the blocks that we've found and the sector hashes associated with that file.
    
    for line in open(os.path.join(dn,"identified_blocks_expanded.txt")):
        (disk_offset,sector_hash,meta) = line.split("\t")
        disk_offset = int(disk_offset)
        filename    = meta.split(",")[1].split('=')[1]

        file_offset = int(meta.split(",")[2].split('=')[1])
        file_block  = file_offset // 4096
        file_blocks[filename].add(file_block)
        file_sector_hashes[filename].add(sector_hash)
        file_offets[filename].append((file_offset,disk_offset,
                                    sector_hash,
                                    sector_hash in h_hashes,
                                    sector_hash in w_hashes,
                                    sector_hash in r_hashes))
        if hash_counts[sector_hash]==1:
            distinct_file_blocks[filename].add(file_block)
    
    # Now sort all of the disk_offset lists
    for filename in file_offets.keys():
        file_offets[filename].sort()

    with open(dbfile,'wb') as f:
        for obj in [file_offets,hash_counts, file_blocks, distinct_file_blocks,file_sector_hashes, h_hashes, w_hashes, r_hashes]:
            pickle.dump(obj, f, pickle.HIGHEST_PROTOCOL)


def report(dbfile,outfile):
    #
    # process the files in order.  
    # Each report output line is a tuple that has score 
    # 
    
    with open(dbfile,'rb') as f:
        file_offsets = pickle.load(f)
        hash_counts = pickle.load(f)
        file_blocks = pickle.load(f)
        distinct_file_blocks = pickle.load(f)
        file_sector_hashes = pickle.load(f)
        h_hashes = pickle.load(f)
        w_hashes = pickle.load(f)
        r_hashes = pickle.load(f)

    if sys.version_info >= (3,0,0):
        of = open(outfile, 'w', newline='')
    else:
        of = open(outfile, 'wb')
    import csv
    ofwriter = csv.writer(of,  dialect='excel')

    # Calculate the number of aligned blocks for each file
    aligned_blocks_per_file = dict()
    for filename in file_offsets:

        # fos is an array sorted by file_offset that contains elements of
        # file_offset, disk_offset, and hash
        fos = file_offsets[filename]

        # of file offsets, A, B, I want to know if there exists , I want to know if there exists 
        # a disk block with the hash of the first 
        # another disk block that has the same disk offset
        # Count the number of aligned blocks
        aligned_blocks = 0
        
        # Try to identify pairs of blocks that have the same delta in file offsets as in
        # disk offsets. Do this only once for each file offset
        total_file_offsets  = set()
        solved_file_offsets = set()
        solved_j_offsets    = set()
        for i in range(0,len(fos)-1):
            # Starting at i, look for the next array entry that has a different file_offset

            a = fos[i]   # this block
            this_file_offset = a[0]
            total_file_offsets.add(this_file_offset)
            if this_file_offset in solved_file_offsets: continue # we've already solved this one

            # Get the next file offset
            b = None
            for j in range(i+1,len(fos)):
                if fos[i][0]==fos[j][0]: continue
                b = fos[j]
                break
            if not b: continue  # there is no next block
            next_file_offset = b[0]

            # Now look for all of the sectors with next_file_offset and see if the delta matches
            for j in range(i+1,len(fos)):
                if j in solved_j_offsets: continue
                if fos[j][0]==next_file_offset and fos[j][0]-a[0]==fos[j][1]-a[1]:
                    # We found a match!
                    solved_file_offsets.add(this_file_offset)
                    solved_j_offsets.add(j)
                    aligned_blocks += 1
                    break
                # If we went too far, then stop
                if fos[j][0]>next_file_offset: 
                    break

        percentage = 0
        seen_blocks = len(file_blocks[filename])
        if aligned_blocks>0 and seen_blocks>1:
            percentage = aligned_blocks / (seen_blocks-1)
        aligned_blocks_per_file[filename] = (aligned_blocks,percentage)


    # Inverse Sort by aligned blocks
    filenames_sorted = sorted(file_offsets,key=lambda a:aligned_blocks_per_file[a][0],reverse=True)
    first = True
    for filename in filenames_sorted:
        b1  = min(file_blocks[filename]) # first block seen
        b2  = max(file_blocks[filename]) # last block seen
        num = len(file_blocks[filename]) # number of blocks seen
        missing = (b2-b1+1)-num          # missing between b1 and b2

        # Tabulate the statistics on this file
        # counts keeps track of the number of 'distinct' counts for each hash in the file
        distinct_count = 0
        distinct_count_uncommon = 0
        total_count_uncommon = 0
        hcount = 0
        wcount = 0
        rcount = 0
        count  = 0
        
        for hash in file_sector_hashes[filename]:
            count += 1
            common = (hash in h_hashes) or (hash in w_hashes) or (hash in r_hashes)
            if not common:
                total_count_uncommon += 1
            if hash_counts[hash]==1:
                distinct_count += 1
                if not common:
                    distinct_count_uncommon += 1
                    
        if distinct_count==0:
            continue

        runs = make_runs(file_blocks[filename])
        lens = [a[1]-a[0]+1 for a in runs]
        longest_run    = max(lens)

        distinct_runs = make_runs(distinct_file_blocks[filename])
        distinct_lens = [a[1]-a[0]+1 for a in distinct_runs]
        longest_distinct_run = max(distinct_lens)
        
        pmatch         = filename
        blocks_seen    = count
        aligned_blocks = aligned_blocks_per_file[filename][0]
        aligned_percent= aligned_blocks_per_file[filename][1]*100.0
        distinct_count = distinct_count
        distinct_percent = distinct_count/count * 100.0
        block_range      = "{}--{}".format(b1,b2)
        
        if sys.version_info >= (3,0,0):
            pmatch=pmatch.replace(u'\U0010001c',"")
        else:
            pmatch=pmatch.replace(b"\xe2\xa0\xc3\xc3\xa3","")

        if first:
            ofwriter.writerow(['Match','Blocks Seen','Range',
                               'Longest Run',
                               'Longest Distinct Run',
                               'Blocks Aligned','% Aligned',
                               'BLocks Distinct','% distinct'])
            first = False
        ofwriter.writerow([pmatch,blocks_seen,block_range,
                           longest_run,
                           longest_distinct_run,
                           aligned_blocks,aligned_percent,distinct_count,distinct_percent])

if __name__=="__main__":
    import sys
    import argparse
    parser = argparse.ArgumentParser()
    
    parser.add_argument("--force",help="regenerate database",action='store_true')
    parser.add_argument("--notaligned",help="show matches that are not aligned",action='store_true')
    parser.add_argument("reportdir",help="Report directory")
    args = parser.parse_args()

    if args.reportdir.endswith("/") or args.reportdir.endswith("\\"):
        args.reportdir = args.reportdir[:-1]
    report_db     = os.path.join(args.reportdir,"blocks.db")
    report_output = os.path.join(args.reportdir,"blocks-report.csv")

    if not os.path.exists(args.reportdir):
        print("{} does not exist".format(args.reportdir))

    if args.force:
        if os.path.exists(report_db):
            os.unlink(report_db)

    explained_fn = os.path.join(args.reportdir,"identified_blocks_explained.txt")
    if os.path.exists(explained_fn):
        process_explained(explained_fn,report_db)

    expanded_fn = os.path.join(args.reportdir,"identified_blocks_expanded.txt")
    if os.path.exists(expanded_fn):
        process_expanded(args.reportdir,report_db)

    if os.path.exists(os.path.join(args.reportdir,"identified_blocks_expanded.txt")):
        process_expanded(args.reportdir,report_db)
        report(report_db,report_output)
        print("Output report in {}".format(report_output))
