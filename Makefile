DESTDIR		=

TARGET		= execpty
TARGETDIR	= $(DESTDIR)/usr/bin

INSTALL		= install -m 755 -p -s -D -t

$(TARGET):

install: $(TARGET) FORCE
	$(INSTALL) "$(TARGETDIR)" "$(TARGET)"

uninstall:
	$(RM) "$(TARGETDIR)/$(TARGET)"

clean:
	$(RM) "$(TARGET)"

FORCE:

