#!/bin/bash
# subsfor the bash scripts
LIBEWF_URL=https://github.com/libyal/libewf-legacy/releases/download/20140814/libewf-20140814.tar.gz
LIBEWF_FNAME=$(basename $LIBEWF_URL)
LIBEWF_DIR=$( echo $LIBEWF_FNAME | sed s/-experimental// | sed s/.tar.gz//)

function make_libewf {
    echo
    echo "Now installing libewf into $LIBEWF_DIR"
    wget -nv $LIBEWF_URL  || (echo could not download $LIBEWF_URL. Stop; exit 1)
    tar xfz $LIBEWF_FNAME || (echo could not untar $LIBEWF_FNAME. Stop; exit 1)
    (cd $LIBEWF_DIR  \
	 && ./configure --quiet --enable-silent-rules --prefix=/usr/local \
	 && make \
	 && sudo make install) || (echo could not build libewf. Stop; exit 1)
    echo Cleaning up $LIBEWF_FNAME and $LIBEWF_DIR
    /bin/rm -rf $LIBEWF_FNAME $LIBEWF_DIR || (echo could not clean up. Stop; exit 1)

    # Make sure that /usr/local/lib is in ldconfig
    sudo /bin/rm -f /tmp/local.conf
    echo /usr/local/lib > /tmp/local.conf
    sudo mv /tmp/local.conf /etc/ld.so.conf.d/local.conf
    sudo ldconfig
}
