#!/bin/bash
# set -x

echo "Installing the real time tool (not the shell builtin)"
dnf install -y time

poweroff
