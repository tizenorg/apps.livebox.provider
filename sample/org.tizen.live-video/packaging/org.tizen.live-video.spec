Name: org.tizen.live-video
Summary: Video playing livebox
Version: 0.0.1
Release: 1
Group: main/app
License: Flora License
Source0: %{name}-%{version}.tar.gz
BuildRequires: cmake, gettext-tools, coreutils
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(provider)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(mm-player)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xdamage)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(libdri2)

%description
App style livebox content provider (video)

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
mkdir -p /opt/usr/live/org.tizen.live-video/libexec
touch /opt/usr/live/org.tizen.live-video/libexec/liblive-org.tizen.live-video.so

%files -n org.tizen.live-video
#%manifest org.tizen.data-provider-slave.manifest
%defattr(-,root,root,-)
/opt/usr/apps/org.tizen.live-video/bin/live-video
/opt/share/packages/*.xml
/opt/usr/live/org.tizen.live-video/etc/*
/opt/usr/live/org.tizen.live-video/res/image/*
/opt/share/icons/default/small/*.png
