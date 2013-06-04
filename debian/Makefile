VERSION=$(shell ./get_version.sh)
MYDIST=$(shell cat /etc/lsb-release | grep CODENAME | sed -e 's/[^=]*[=]//')
SUBVERSION=1
PACKAGE=backtobasics
DEPENDS=libpcre3,elementals,libstreamripper,libmad0,libid3tag0,libvorbisfile3,libao4,libflac8,libmpg123-0,libgtklistmodel,libgtk-3-0,libwebkitgtk-3.0-0,libtagcoverart,libtagc0,libcurl3-gnutls,libappindicator3-0.1
DEPENDS_QUANTAL=libconfig9
DEPENDS_PRECISE=libconfig8
DEPENDS1=
BUILDDEPENDS=gawk,automake,autoconf,intltool,libpthread-stubs0-dev,libpcre3-dev,pkg-config,elementals,libstreamripper,libglib2.0-dev,libao-dev,libflac-dev,libmpg123-dev,libmad0-dev,libid3tag0-dev,libvorbis-dev,libgtk-3-dev,libwebkitgtk-3.0-dev,libtag1-dev,libtagcoverart,libtagc0-dev,libconfig8-dev,libcurl3-gnutls-dev,libgtklistmodel,libappindicator3-0.1-dev
DISTROS=precise quantal raring saucy
#DISTROS=raring saucy
#DISTROS=precise
DISTROS1=maverick lucid
DVERSION=${VERSION}.${DISTRO}.${SUBVERSION}
UBUNTU=/tmp/backtobasics_ppa
UDIR=${UBUNTU}/${PACKAGE}-${DVERSION}
DDIR=${UDIR}/debian
PPA=ppa:0k-hans-f8/ppa

all:
	@echo "make srctar - makes a source tar which needs automake/autoconf"
	@echo ""
	@echo "make ppa - makes a PPA for launchpad for Ubuntu's distros:"
	@echo ""
	@echo "       ${DISTROS}"
	@echo ""
	@echo "make ppa_old - makes a PPA for launchpad for Ubuntu's distros:"
	@echo ""
	@echo "       ${DISTROS1}"
	@echo ""

srctar: 
	rm -rf /tmp/${PACKAGE}-${VERSION} 
	if [ -r ${PACKAGE}/Makefile ];then (cd ${PACKAGE};make distclean); fi
	mkdir /tmp/${PACKAGE}-${VERSION}
	(cd ..;tar cf - . ) | (cd /tmp/${PACKAGE}-${VERSION}; tar xf -)
	(cd /tmp/${PACKAGE}-${VERSION};automake;autoconf)
	(cd /tmp;tar -c -z --exclude .git -f ${PACKAGE}-${VERSION}.tar.gz ${PACKAGE}-${VERSION})
	mv /tmp/${PACKAGE}-${VERSION}.tar.gz .
	rm -rf /tmp/${PACKAGE}-${VERSION} 

clean:
	rm -f *.tar.gz
	rm -rf ${UBUNTU}

test: clean
	make build DISTRO=${MYDIST} DEPEND=${DEPENDS}
	(cd ${UBUNTU};sudo pbuilder build *.dsc)

ppa: clean
	for distro in ${DISTROS}; do make clean;if [ "$$distro" = "quantal" -o "$$distro" = "raring" -o "$$distro" = "saucy" ]; then make ppa1 DISTRO=$$distro DEPEND="${DEPENDS},${DEPENDS_QUANTAL}"; else make ppa1 DISTRO=$$distro DEPEND="${DEPENDS},${DEPENDS_PRECISE}"; fi; done

ppa_old: clean
	for distro in ${DISTROS1}; do make ppa1 DISTRO=$$distro DEPEND=${DEPENDS1}; done

ppa1: build
	(cd ${UBUNTU};dput ${PPA} *.changes)

build: srctar
	echo Making PPA for ${DISTRO}
	rm -rf ${UBUNTU}
	mkdir ${UBUNTU}
	cp ${PACKAGE}-${VERSION}.tar.gz ${UBUNTU}/${PACKAGE}_${DVERSION}.orig.tar.gz
	(cd ${UBUNTU}; tar xzf ${PACKAGE}_${DVERSION}.orig.tar.gz)
	mv ${UBUNTU}/${PACKAGE}-${VERSION} ${UDIR}
	tar cf - debian | (cd ${UDIR}; tar xf -)
	cat ${DDIR}/control.in | sed -e 's/%%DEPENDS%%/${DEPEND}/g' | sed -e 's/%%BUILDDEPENDS%%/${BUILDDEPENDS}/g' >${DDIR}/control
	rm ${DDIR}/control.in
	cat ${DDIR}/changelog.in | sed -e 's/%%VERSION%%/${VERSION}/g' | sed -e 's/%%DISTRO%%/${DISTRO}/g' | sed -e 's/%%SUBVERSION%%/${SUBVERSION}/g' >${DDIR}/changelog
	rm ${DDIR}/changelog.in
	(cd ${UDIR};debuild -S)




