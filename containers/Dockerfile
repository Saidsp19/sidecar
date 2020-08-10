# -*- Docker -*-
FROM centos:8

ARG DOCKER_USER
ENV LC_ALL C.UTF-8
USER root

WORKDIR /

RUN yum update -y && yum install -y epel-release
RUN yum install -y 'dnf-command(config-manager)'
RUN yum config-manager --set-enabled PowerTools
RUN yum update -y

RUN yum install -y \
    curl \
    sudo \
    emacs \
    openssl-devel \
    avahi-compat-libdns_sd-devel \
    autoconf \
    automake \
    gcc \
    gcc-c++ \
    ccache \
    make \
    cmake \
    git \
    python3 \
    nss-mdns \
    boost \
    boost-devel \
    fftw \
    fftw-devel \
    freeglut-devel \
    lapack-devel \
    libpng-devel \
    qt5-devel \
    qt5-qtbase \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    qt5-qtsvg \
    qt5-qtsvg-devel

# ACE library
RUN rpm -i https://download.opensuse.org/repositories/devel:/libraries:/ACE:/micro/CentOS_8/x86_64/ace-6.5.10-81.3.x86_64.rpm
RUN rpm -i https://download.opensuse.org/repositories/devel:/libraries:/ACE:/micro/CentOS_8/x86_64/ace-devel-6.5.10-81.3.x86_64.rpm

# -- Install gosu
RUN gpg --keyserver ha.pool.sks-keyservers.net --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4 && \
    curl -o /usr/local/bin/gosu -SL "https://github.com/tianon/gosu/releases/download/1.2/gosu-amd64" && \
    curl -o /usr/local/bin/gosu.asc -SL "https://github.com/tianon/gosu/releases/download/1.2/gosu-amd64.asc" && \
    gpg --verify /usr/local/bin/gosu.asc && \
    rm /usr/local/bin/gosu.asc && \
    chmod +x /usr/local/bin/gosu && \
    echo "ALL ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers

WORKDIR /opt

RUN git clone https://github.com/bradhowes/openvsip && \
    cd openvsip && \
    ./autogen.sh && \
    mkdir objdir && \
    cd objdir && \
    ../configure && \
    make && \
    make install && \
    cd / && \
    rm -rf /opt/openvsip

COPY profile.d /etc/profile.d
RUN chmod 755 /etc/profile.d/*
COPY entrypoint.sh /usr/local/bin/entrypoint

# Creaate container user
RUN groupadd ${DOCKER_USER} && \
    useradd -d /home/${DOCKER_USER} -g ${DOCKER_USER} -ms /bin/bash ${DOCKER_USER} && \
    usermod -aG wheel ${DOCKER_USER}

COPY user_data/ /tmp/user_data
RUN chmod -R 755 /tmp/user_data && \
    cp -rp /tmp/user_data/. /home/${DOCKER_USER}/ && \
    mkdir -p /home/${DOCKER_USER}/sidecar && \
    chown -R ${DOCKER_USER}:${DOCKER_USER} /home/${DOCKER_USER} && \
    chmod -R a+x /usr/local/bin/entrypoint /home/${DOCKER_USER}/bin && \
    alternatives --set python /usr/bin/python3

WORKDIR /home/${DOCKER_USER}

STOPSIGNAL SIGRTMIN+3
CMD [ "/sbin/init" ]
