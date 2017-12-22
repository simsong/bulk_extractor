# Bulk Extractor with Record Carving (bulk_extractor-rec)

It is based on [bulk_extrator](https://github.com/simsong/bulk_extractor) and supports following scanner: 

* ntfsindx - $INDEX_ALLOCATION record (INDX)
* ntfslogfile - $LogFile record (RSTR/RCRD)
* ntfsmft - $MFT record (FILE)
* ntfsusn - $UsnJrnl:$J record (USN_RECORD_V2/V4) 
* utmp - wtmp/btmp record (utmp)

## Ready to build

The following procedure works on Fedora 26 or above.

### Install required package

```
sudo dnf update
sudo dnf groupinstall development-tools
sudo dnf install flex zlib-devel
sudo dnf install libxml2-devel compat-openssl10-devel tre-devel bzip2-devel libtool gcc-c++
sudo dnf install libewf-devel afflib-devel sqlite-devel --skip-broken
sudo dnf install java-1.8.0-openjdk-devel
```

### Git

```
git clone https://github.com/4n6ist/bulk_extractor.git
cd bulk_extractor
git checkout dev-rec
sh bootstrap.sh
```

## Build

### Windows (exe)

```
cd src_win
./CONFIGURE_F20.bash
make
```

If you encounter an error at libewf when you run CONFIGURE_F20.bash , then edit libewf-########/libuna/libuna_extern.h

```
(Line 44)
#define LIBUNA_EXTERN /* extern */
->
#define LIBUNA_EXTERN extern
```

Then try to build/install libewf and run CONFIGURE_F20.bash again.

```
cd libewf-########
make
sudo make install
cd ..
./CONFIGURE_F20.bash
make
```

### Linux

```
./configure
make
sudo make install
```
## Documentation & Download

Documentation and windows binary is available at https://www.kazamiya.net/bulk_extractor-rec/

