Name:		streamget
Version:	1.4
Release:	1
Summary:	Robust stream recorder. Does automatic reconnect when stream is temporarily unavailable.
Group:		Applications/Multimedia
License:	Proprietary
URL:		http://www.audioserver.nl
Vendor:		AUDIOserver.nl <info@audioserver.nl>
Source:     	%{name}-%{version}.tar.gz
Prefix:		%{_prefix}
BuildRoot:	%{_tmppath}/%{name}-root

Requires:       curl >= 7.10.0
BuildRequires:	curl-devel >= 7.10.0

%description
Streamget is a robust stream recorder that is independent of the stream type.
It supports only HTTP streams. It has been tested with Icecast2 streams but
may work with other streams too. Streamget simply downloads the data from a
specified URL. If the connection is broken it reconnects and appends the
output to the data already downloaded.

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --mandir=%{_mandir} --sysconfdir=%{_prefix}/etc
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install
rm -rf $RPM_BUILD_ROOT%{_datadir}/doc/%{name}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%pre

# this section is run before the installation of files
# during install (-i) and upgrade (-U) unless --noscripts was specified

%post

# this section is run after the installation of files
# during install (-i) and upgrade (-U) unless --noscripts was specified

%preun

# this section is run before files are being uninstalled
# during erase (-e) and upgrade (-U)

%postun

# this section is run after files have been uninstalled
# during erase (-e) and upgrade (-U)

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING NEWS ChangeLog
%{_bindir}/streamget

%changelog
# See ChangeLog
