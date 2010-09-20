libtoolize -c
autoheader
aclocal
automake --add-missing -c
autoconf
./configure --prefix=/usr
make
make install
