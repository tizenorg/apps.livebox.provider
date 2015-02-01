%bcond_with wayland

Name: libdynamicbox_provider
Summary: Library for developing the dynamicbox service provider
Version: 1.0.0
Release: 1
Group: HomeTF/DynamicBox
License: Flora
Source0: %{name}-%{version}.tar.gz
Source1001: %{name}.manifest

BuildRequires: cmake, gettext-tools, coreutils
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(com-core)
BuildRequires: pkgconfig(dynamicbox_service)

BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: model-build-features

%if %{with wayland}
%else
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xext)
BuildRequires: pkgconfig(libdri2)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(xfixes)
BuildRequires: pkgconfig(dri2proto)
BuildRequires: pkgconfig(xdamage)
%endif

%description
Supporting the commnuncation channel with master service for dynamicbox remote view.
API for accessing the remote buffer of dynamicboxes.
Feature for life-cycle management by the master provider.

%package devel
Summary: Dynamicbox data provider development library (dev)
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
Header & package configuration files for developing the dynamicbox service provider

%prep
%setup -q
cp %{SOURCE1001} .

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="${CFLAGS} -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="${CXXFLAGS} -DTIZEN_ENGINEER_MODE"
export FFLAGS="${FFLAGS} -DTIZEN_ENGINEER_MODE"
%endif

%if %{with wayland}
export WAYLAND_SUPPORT=On
export X11_SUPPORT=Off
%else
export WAYLAND_SUPPORT=Off
export X11_SUPPORT=On
%endif

%cmake . -DWAYLAND_SUPPORT=${WAYLAND_SUPPORT} -DX11_SUPPORT=${X11_SUPPORT}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/%{_datarootdir}/license

%post -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%files -n %{name}
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/%{name}.so*
%{_datarootdir}/license/%{name}

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/dynamicbox_provider/dynamicbox_provider.h
%{_includedir}/dynamicbox_provider/dynamicbox_provider_buffer.h
%{_libdir}/pkgconfig/dynamicbox_provider.pc


#################################################
# libdynamicbox_provider_app
%package -n %{name}_app
Summary: Library for developing the dynamicbox app provider
Group: HomeTF/Dynamicbox
License: Flora
Requires: libdynamicbox_provider

%description -n %{name}_app
Provider APIs to develop the dynamicbox provider applications.

%package -n %{name}_app-devel
Summary: Header & package configuration files to support development of the dynamicbox provider applications.
Group: Development/Libraries
Requires: %{name}_app

%description -n %{name}_app-devel
Dynamicbox provider application development library (dev)

%files -n %{name}_app
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/%{name}_app.so*
%{_datarootdir}/license/%{name}_app

%files -n %{name}_app-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/dynamicbox_provider_app/dynamicbox_provider_app.h
%{_libdir}/pkgconfig/dynamicbox_provider_app.pc

#################################################
# libprovider (for old version)
%package -n libprovider
Summary: Library for developing the dynamicbox app provider (old version)
Group: HomeTF/Dynamicbox
License: Flora
Requires: libdynamicbox_provider

%description -n libprovider
Provider APIs to develop the dynamicbox provider applications. (old version)

%package -n libprovider-devel
Summary: Header & package configuration files to support development of the dynamicbox provider applications. (old version)
Group: Development/Libraries
Requires: libprovider

%description -n libprovider-devel
Dynamicbox provider application development library (dev) (old version)

%files -n libprovider
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libprovider.so*
%{_datarootdir}/license/libprovider

%files -n libprovider-devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/provider/provider.h
%{_includedir}/provider/provider_buffer.h
%{_libdir}/pkgconfig/provider.pc

# End of a file
