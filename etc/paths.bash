LIBEWF_URL=https://github.com/libyal/libewf-legacy/releases/download/20140814/libewf-20140814.tar.gz
LIBEWF_FNAME=$(basename $LIBEWF_URL)
LIBEWF_DIR=$( echo $LIBEWF_FNAME | sed s/-experimental// | sed s/.tar.gz//)
