DESTDIR		=

TARGET		= execpty
TARGETDIR	= $(DESTDIR)/usr/bin
INSTALL		= install -m 755 -p -s -D -t

MANPAGE		= execpty.1.gz
MANDIR		= $(DESTDIR)/usr/share/man/man1
MANINSTALL	= install -m 644 -p -D -t


$(TARGET):

install: $(TARGET) FORCE
	$(INSTALL) "$(TARGETDIR)" "$(TARGET)"
	$(MANINSTALL) "$(MANDIR)" "$(MANPAGE)"

uninstall:
	$(RM) "$(TARGETDIR)/$(TARGET)"
	$(RM) "$(MANDIR)/$(MANPAGE)"

clean:
	$(RM) "$(TARGET)"

FORCE:

