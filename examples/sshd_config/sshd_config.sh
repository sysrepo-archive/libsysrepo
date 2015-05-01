pyang -f sample-skeleton --sample-skeleton-defaults sshd_config.yang > sshd_config.xml
xsltproc sshd_config.xsl sshd_config.xml > sshd_config
