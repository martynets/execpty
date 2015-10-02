TARGET		= execpty
INSTALL_DIR	= /usr/bin

STRIP		= strip
INSTALL		= install -m 755 -p

$(TARGET):

install: $(TARGET) FORCE
	$(INSTALL) "$(TARGET)" "$(INSTALL_DIR)/$(TARGET)"
	$(STRIP) "$(INSTALL_DIR)/$(TARGET)"

uninstall:
	$(RM) "$(INSTALL_DIR)/$(TARGET)"

FORCE:

