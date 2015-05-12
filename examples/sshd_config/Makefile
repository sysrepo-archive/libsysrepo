MODULES = sshd_config.yang
TARGET ?= config
BASE ?= sshd
bn = $(BASE)-$(TARGET)
schemas = $(bn).rng $(BASE)-gdefs-config.rng $(bn)-dsrl.xsl $(bn)-sch.xsl
tgt = --stringparam target $(TARGET)
rngparms =  $(tgt) --stringparam basename $(BASE) --stringparam schema-dir \
	$(PYANG_RNG_LIBDIR) $(PYANG_XSLT_DIR)/gen-relaxng.xsl
Y2DOPTS = -t $(TARGET) -b $(BASE)
SKELOPTS = --sample-xml-skeleton-defaults --sample-xml-skeleton-doctype=config
SSHSS = sshd_config.xsl
.PHONY = all clean rnc skel tree

all: $(schemas)

rnc: $(bn).rnc

skel: $(bn)-skel.xml

xml: $(bn)-skel.xml

$(bn)-skel.xml: $(MODULES)
	@pyang -f sample-xml-skeleton $(SKELOPTS) $^ | \
	    xmllint --output $@ --format -
	@cp $(bn)-skel.xml $(bn).xml

model.tree: $(MODULES)
	@pyang -o $@ -f tree $^

$(bn).dsdl: $(MODULES)
	@pyang -f dsdl -o $@ --dsdl-no-documentation --dsdl-no-dublin-core $^

$(bn).rng: $(bn).dsdl
	@xsltproc --output $@ $(rngparms) $<

$(BASE)-gdefs-config.rng: $(bn).dsdl
	@xsltproc --output $@ --stringparam gdefs-only 1 $(rngparms) $<

$(bn).rnc: $(bn).rng $(BASE)-gdefs-config.rng
	@trang -I rng -O rnc $< $@

$(bn)-dsrl.xsl: $(bn).dsdl
	@xsltproc $(tgt) $(PYANG_XSLT_DIR)/gen-dsrl.xsl $< | \
	    xsltproc --output $@ $(PYANG_XSLT_DIR)/dsrl2xslt.xsl -

$(bn)-sch.xsl: $(bn).dsdl
	@xsltproc $(tgt) $(PYANG_XSLT_DIR)/gen-schematron.xsl $< | \
	    xsltproc $(PYANG_XSLT_DIR)/iso_abstract_expand.xsl - | \
            xsltproc -o $@ $(PYANG_XSLT_DIR)/iso_svrl_for_xslt1.xsl -

validate: $(bn)-skel.xml $(schemas)
	jing $(bn).rng $<
	xsltproc $(bn)-dsrl.xsl $< | xsltproc $(bn)-sch.xsl - | \
	    xsltproc $(PYANG_XSLT_DIR)/svrl2text.xsl -

sshd_config: $(bn).xml
	@xsltproc -o $@ $(SSHSS) $<

clean:
	@rm -f $(schemas) $(bn)-skel.xml $(bn).xml sshd_config $(bn).dsdl \
	    model.tree *.rnc
