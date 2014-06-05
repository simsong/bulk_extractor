#!/bin/bash
DATA1=Data-1.4.0 
DATA2=Data-1.5.0
python3 regress.py --datadir Data --exe ~/bin/bulk_extractor-1.4.0 --outdir=$DATA1 --no-find
python3 regress.py --datadir Data --exe ../src/bulk_extractor      --outdir=$DATA2 --no-find
python3 regress.py --datadircomp $DATA1,$DATA2
