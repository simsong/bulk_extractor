# bulk_extractor Makefile.am
# This file is compiled with automake to create Makefile.in.
# Makefile.in is transformed by "configure" to create Makefile
#
# (C) 2020 Simson L. Garfinkel
# https://www.gnu.org/licenses/lgpl-3.0.en.html

# We need to list *all* of the subdirs here that have Makefiles in them that are executed by
# doing a chdir into the directory and typing 'make'. So we do not need src/be20_api
SUBDIRS = doc doc/latex_manuals man src src/tests specfiles tests

# Include things explicitly
# Note - autoconf seems to automatically include the m4 rules that are used by configure.ac - no need to list them heren
#
# Don't include bootstrap. People run it, and they shouldn't
# It's only for people who check out the git repo
#
# note - we are allowed to use wildcards below! Who knew.
# and using $wildcard prevents it from running on mac, which doesn't use gnu make

include Makefile.auto_defs

EXTRA_DIST = $(SRC_WIN_DIST) $(AUTO_DOC_FILES) $(AUTO_ETC_FILES) $(AUTO_LICENSES) \
	$(srcdir)/etc/Dockerfile.fedora-distcheck \
	$(srcdir)/etc/Dockerfile.ubuntu-distcheck \
	$(srcdir)/cloudformation/release-large-image-validation.json \
	$(srcdir)/scripts/assemble_github_release.py \
	$(srcdir)/scripts/check_aws_large_image_template.py \
	$(srcdir)/scripts/release_aws_large_image.sh \
	$(srcdir)/scripts/aws_large_image_worker.sh \
	$(srcdir)/scripts/release.sh \
	$(srcdir)/AGENTS.md \
	$(srcdir)/.gitignore \
	$(srcdir)/CODING_STANDARDS.md \
	$(srcdir)/LICENSE.md \
	$(srcdir)/README.md \
	$(srcdir)/Dockerfile \
	$(srcdir)/.dockerignore \
	$(srcdir)/dfxml_schema/LICENSE.md \
	$(srcdir)/dfxml_schema/README.md \
	$(srcdir)/dfxml_schema/VENDORED.md \
	$(srcdir)/dfxml_schema/dfxml.xsd \
	$(srcdir)/dfxml_schema/ref/dc.xsd \
	$(srcdir)/dfxml_schema/ref/xml.xsd


# ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

distclean2:
	@echo Deleting:
	@find . \
        \( -name '*.orig' -o -name '*.rej' -o -name '*~' \
        -o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
        -o -name '.*.rej' -o -size 0 \
        -o -name '*%' -o -name '.*.cmd' -o -name 'core' \) \
        -type f -print | tee /dev/tty | xargs rm -f

.PHONY: exels
exels:
	/bin/ls -l */*exe

.PHONY: execlean
execlean:
	/bin/rm -f */*exe

.PHONY: win64
win64: win64/bulk_extractor64.exe

win64/bulk_extractor64.exe:
	@echo '****************************************************'
	@echo '*** making win64 with mingw32-configure and make ***'
	@echo '****************************************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure) ; fi
	(cd win64;$(MAKE))
	(cp win64/src/bulk_extractor.exe win64/bulk_extractor64.exe)
	ls -l win64/bulk_extractor64.exe

winlibs:
	@echo '*****************************'
	@echo '*** making win64 library  ***'
	@echo '*****************************'
	if [ -r Makefile ]; then $(MAKE) distclean ; fi
	if [ ! -d win64  ]; then mkdir win64 ; fi
	if [ ! -r win64/Makefile ]; then (cd win64;mingw64-configure --disable-libewf --disable-afflib) ; fi
	(cd win64/src;$(MAKE) lib;mv libbulkextractor.so ../../libbulkextractor64.DLL)
	ls -l libbulk*.DLL

.PHONY: lib
lib:
	(cd src; $(MAKE) lib)

.PHONY: libinstall
libinstall:
	install -D src/libbulkextractor.so $(prefix)/lib/libbulkextractor.so
	install -D python/module/bulkextractor.py $(prefix)/lib/python2.7/dist-packages/bulkextractor.py
	if [ -r /sbin/ldconfig ]; /sbin/ldconfig -v ; fi

# Release orchestration is deliberately fail-closed and isolated from the
# active checkout. See scripts/release.sh and doc/RELEASE_PROCEDURE.md.
RELEASE_ARTIFACT_DIR ?= $(abs_builddir)/release-artifacts
RELEASE_SCRIPT = $(abs_srcdir)/scripts/release.sh
RELEASE_TAG ?=
AWS_LARGE_IMAGE_TEMPLATE = $(abs_srcdir)/cloudformation/release-large-image-validation.json
AWS_LARGE_IMAGE_SCRIPT = $(abs_srcdir)/scripts/release_aws_large_image.sh
.PHONY: release
release:
	RELEASE_ARTIFACT_DIR="$(RELEASE_ARTIFACT_DIR)" RELEASE_SOURCE_DIR="$(abs_srcdir)" "$(RELEASE_SCRIPT)"

.PHONY: release-aws-large-image release-aws-large-image-template-check release-aws-large-image-cloudformation-check
release-aws-large-image:
	AWS_LARGE_IMAGE_TEMPLATE="$(AWS_LARGE_IMAGE_TEMPLATE)" "$(AWS_LARGE_IMAGE_SCRIPT)"

release-aws-large-image-template-check:
	python3 "$(abs_srcdir)/scripts/check_aws_large_image_template.py" "$(AWS_LARGE_IMAGE_TEMPLATE)"

release-aws-large-image-cloudformation-check:
	aws cloudformation validate-template --template-body "file://$(AWS_LARGE_IMAGE_TEMPLATE)" >/dev/null

.PHONY: release-deb
release-deb:
	test -n "$(RELEASE_TAG)"
	test "$$(git -C "$(abs_srcdir)" cat-file -t "refs/tags/$(RELEASE_TAG)")" = tag
	test "$$(git -C "$(abs_srcdir)" rev-parse "$(RELEASE_TAG)^{commit}")" = "$$(git -C "$(abs_srcdir)" rev-parse HEAD)"
	git -C "$(abs_srcdir)" diff --quiet
	git -C "$(abs_srcdir)" diff --cached --quiet
	test -z "$$(git -C "$(abs_srcdir)" ls-files --others --exclude-standard)"
	mkdir -p "$(RELEASE_ARTIFACT_DIR)"
	$(CONTAINER_ENGINE) build --progress=plain --file "$(abs_srcdir)/etc/Dockerfile.debian-package" --tag bulk-extractor-deb-build "$(abs_srcdir)"
	cid=$$($(CONTAINER_ENGINE) create bulk-extractor-deb-build); \
	  $(CONTAINER_ENGINE) cp "$$cid":/ - | tar -xf - --wildcards --strip-components=1 \
	    -C "$(RELEASE_ARTIFACT_DIR)" 'bulk-extractor_*'; \
	  $(CONTAINER_ENGINE) rm "$$cid"

# Each Finch build copies a clean source context and runs the real distcheck
# inside it. Finch selects native arm64 and emulated amd64 on Apple Silicon.
CONTAINER_ENGINE ?= finch
CONTAINER_PLATFORMS ?= arm64 amd64

.PHONY: distcheck-ubuntu-container distcheck-fedora-container distcheck-containers
distcheck-ubuntu-container:
	@set -e; for platform in $(CONTAINER_PLATFORMS); do \
		$(CONTAINER_ENGINE) build --platform "$$platform" --progress=plain \
			--file etc/Dockerfile.ubuntu-distcheck .; \
	done

distcheck-fedora-container:
	@set -e; for platform in $(CONTAINER_PLATFORMS); do \
		$(CONTAINER_ENGINE) build --platform "$$platform" --progress=plain \
			--file etc/Dockerfile.fedora-distcheck .; \
	done

distcheck-containers: distcheck-ubuntu-container distcheck-fedora-container
