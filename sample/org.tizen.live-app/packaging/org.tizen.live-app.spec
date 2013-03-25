Name: org.tizen.live-app
Summary: App style livebox
Version: 0.0.1
Release: 1
Group: main/app
License: Flora License
Source0: %{name}-%{version}.tar.gz
BuildRequires: cmake, gettext-tools
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(provider)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(mm-player)

%description
App style livebox content provider

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
#-fpie  LDFLAGS="${LDFLAGS} -pie -O3"
CFLAGS="-Wall -Winline -Werror -p -g" make %{?jobs:-j%jobs}
#${CFLAGS}

%install
rm -rf %{buildroot}
%make_install

%post
mkdir -p /opt/usr/live/org.tizen.live-app/libexec
touch /opt/usr/live/org.tizen.live-app/libexec/liblive-org.tizen.live-app.so

%files -n org.tizen.live-app
#%manifest org.tizen.live-app.manifest
%defattr(-,root,root,-)
/opt/usr/apps/org.tizen.live-app/bin/live-app
/opt/share/packages/*.xml
/opt/usr/live/org.tizen.live-app/etc/*
/opt/usr/live/org.tizen.live-app/res/image/*
/opt/share/icons/default/small/*.png
