# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2
# $Header: $

DESCRIPTION="A simple block-pushing game"
HOMEPAGE="http://kenn.frap.net/wakkabox/"
SRC_URI="http://kenn.frap.net/wakkabox/${P}.tar.gz"
LICENSE="GPL-2"

SLOT="0"

# It shouldn't matter, but hell if I know
KEYWORDS="~x86"

IUSE=""
DEPEND=">=libsdl-1.0.1"
#RDEPEND=""

S=${WORKDIR}/${P}

src_compile() {
	econf || die "./configure failed"
	emake || die
}

src_install() {
	einstall || die
}
