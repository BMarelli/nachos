FROM ubuntu:23.10

SHELL ["/bin/bash", "-c"]

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gcc-mipsel-linux-gnu \
        bear \
        gdb \
    && apt-get clean -y

# Create a new user and give them sudo rights
ARG USER=vscode
RUN useradd -d /home/$USER -s /bin/bash -m $USER
RUN echo "$USER ALL=NOPASSWD: ALL" >> /etc/sudoers
USER $USER
ENV HOME /home/$USER
WORKDIR $HOME

# Run bear to generate compile_commands.json
CMD bear -- make