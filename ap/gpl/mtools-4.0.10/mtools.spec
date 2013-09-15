Summary: mtools, read/write/list/format DOS disks under Unix
Name: mtools
Version: 4.0.10
Release: 1
Group: Utilities/System
URL: http://mtools.linux.lu
Source0: mtools-%{version}.tar.gz
#Patch1: mtools-%{version}-20071226.diff.gz
Buildroot: %{_tmppath}/%{name}-%{version}-buildroot
License: GPL
%description
Mtools is a collection of utilities to access MS-DOS disks
from Unix without mounting them. It supports Win'95 style
long file names, OS/2 Xdf disks, ZIP/JAZ disks and 2m
disks (store up to 1992k on a high density 3 1/2 disk).


%prep
%setup -q
#%patch1 -p1
./configure --prefix=%{buildroot}%{_prefix} --sysconfdir=/etc --infodir=%{buildroot}%{_infodir} --mandir=%{buildroot}%{_mandir} --enable-floppyd

%build
make

%clean
[ X%{buildroot} != X ] && [ X%{buildroot} != X/ ] && rm -r %{buildroot}

%install
make install
make install-info
strip %{buildroot}%{_bindir}/mtools %{buildroot}%{_bindir}/mkmanifest %{buildroot}%{_bindir}/floppyd
rm %{buildroot}%{_infodir}/dir

%files
%defattr(-,root,root)
%{_infodir}/mtools.info*
%{_mandir}/man1/floppyd.1*
%{_mandir}/man1/floppyd_installtest.1.gz
%{_mandir}/man1/mattrib.1*
%{_mandir}/man1/mbadblocks.1*
%{_mandir}/man1/mcat.1*
%{_mandir}/man1/mcd.1*
%{_mandir}/man1/mclasserase.1*
%{_mandir}/man1/mcopy.1*
%{_mandir}/man1/mdel.1*
%{_mandir}/man1/mdeltree.1*
%{_mandir}/man1/mdir.1*
%{_mandir}/man1/mdu.1*
%{_mandir}/man1/mformat.1*
%{_mandir}/man1/minfo.1*
%{_mandir}/man1/mkmanifest.1*
%{_mandir}/man1/mlabel.1*
%{_mandir}/man1/mmd.1*
%{_mandir}/man1/mmount.1*
%{_mandir}/man1/mmove.1*
%{_mandir}/man1/mpartition.1*
%{_mandir}/man1/mrd.1*
%{_mandir}/man1/mren.1*
%{_mandir}/man1/mshowfat.1*
%{_mandir}/man1/mtools.1*
%{_mandir}/man5/mtools.5*
%{_mandir}/man1/mtoolstest.1*
%{_mandir}/man1/mtype.1*
%{_mandir}/man1/mzip.1*
%{_bindir}/amuFormat.sh
%{_bindir}/mattrib
%{_bindir}/mbadblocks
%{_bindir}/mcat
%{_bindir}/mcd
%{_bindir}/mclasserase
%{_bindir}/mcopy
%{_bindir}/mdel
%{_bindir}/mdeltree
%{_bindir}/mdir
%{_bindir}/mdu
%{_bindir}/mformat
%{_bindir}/minfo
%{_bindir}/mkmanifest
%{_bindir}/mlabel
%{_bindir}/mmd
%{_bindir}/mmount
%{_bindir}/mmove
%{_bindir}/mpartition
%{_bindir}/mrd
%{_bindir}/mren
%{_bindir}/mshowfat
%{_bindir}/mtools
%{_bindir}/mtoolstest
%{_bindir}/mtype
%{_bindir}/mzip
%{_bindir}/floppyd
%{_bindir}/floppyd_installtest
%{_bindir}/mcheck
%{_bindir}/mcomp
%{_bindir}/mxtar
%{_bindir}/tgz
%{_bindir}/uz
%{_bindir}/lz

%pre
groupadd floppy 2>/dev/null || echo -n ""

%post
if [ -f %{_bindir}/install-info ] ; then
	if [ -f %{_infodir}/dir ] ; then
		%{_bindir}/install-info %{_infodir}/mtools.info %{_infodir}/dir
	fi
	if [ -f %{_infodir}/dir.info ] ; then
		%{_bindir}/install-info %{_infodir}/mtools.info %{_infodir}/dir.info
	fi
fi


%preun
install-info --delete %{_infodir}/mtools.info %{_infodir}/dir.info
if [ -f %{_bindir}/install-info ] ; then
	if [ -f %{_infodir}/dir ] ; then
		%{_bindir}/install-info --delete %{_infodir}/mtools.info %{_infodir}/dir
	fi
	if [ -f %{_infodir}/dir.info ] ; then
		%{_bindir}/install-info --delete %{_infodir}/mtools.info %{_infodir}/dir.info
	fi
fi
