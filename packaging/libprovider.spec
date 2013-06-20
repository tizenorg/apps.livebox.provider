Name: libprovider
Summary: Library for developing the livebox service provider.
Version: 0.9.3
Release: 1
Group: HomeTF/Livebox
License: Flora License
Source0: %{name}-%{version}.tar.gz
BuildRequires: cmake, gettext-tools, coreutils
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(com-core)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xext)
BuildRequires: pkgconfig(libdri2)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(xfixes)
BuildRequires: pkgconfig(dri2proto)
BuildRequires: pkgconfig(xdamage)
BuildRequires: pkgconfig(livebox-service)

%description
Supporting the commnuncation channel with master service for livebox remote view.
API for accessing the remote buffer of liveboxes.
Feature for life-cycle management by the master provider.

%package devel
Summary: Header & package configuration files for developing the livebox service provider 
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
Livebox data provider development library (dev)

%prep
%setup -q

%build
%cmake .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/%{_datarootdir}/license

%post

%files -n libprovider
%manifest libprovider.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*
%{_datarootdir}/license/*

%files devel
%defattr(-,root,root,-)
%{_includedir}/provider/provider.h
%{_includedir}/provider/provider_buffer.h
%{_libdir}/pkgconfig/*.pc

# End of a file
