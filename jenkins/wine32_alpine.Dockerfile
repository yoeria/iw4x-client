# Requires a decent modern Docker version (v1.10.x at least ideally)

# Use semi-official Arch Linux image with fixed versioning
FROM alpine

# Environment variables
ENV WINEPREFIX /wine32
ENV WINEARCH win32
ENV WINEDEBUG -all

# Install Wine (32-bit)
RUN \
        echo "x86" > /etc/apk/arch &&\
        apk update &&\
        apk upgrade &&\
        apk add \
        wine \
        wget \
        gnutls \
        gnutls-c++ \
        gnutls-utils \
        xvfb \
        xvfb-run \
        samba-winbind-clients
RUN     wine wineboot -i &&\
        wget -Ovcredist_x86.exe https://download.microsoft.com/download/5/B/C/5BC5DBB3-652D-4DCE-B14A-475AB85EEF6E/vcredist_x86.exe &&\
        WINEDEBUG=+all-trace xvfb-run sh -c 'wine vcredist_x86.exe /q' &&\
        rm vcredist_x86.exe &&\
        \
        apk del \
        xvfb \
        xvfb-run \
        wget \
        &&\
        \
        find /. -name "*~" -type f -delete &&\
        rm -rf /tmp/* /var/tmp/* /usr/share/man/* /usr/share/info/* /usr/share/doc/* &&\
        apk del

USER 0
