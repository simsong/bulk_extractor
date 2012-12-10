EXTRA_DIST=bulk_extractor.opensuse.spec bulk_extractor.fedora.spec 
CLEANFILES=bulk_extractor.opensuse.spec bulk_extractor.fedora.spec

bulk_extractor.opensuse.spec: bulk_extractor.spec.m4
	m4 -DOPENSUSE bulk_extractor.spec.m4 > bulk_extractor.opensuse.spec

bulk_extractor.fedora.spec: bulk_extractor.spec.m4
	m4 -DFEDORA bulk_extractor.spec.m4 > bulk_extractor.fedora.spec

