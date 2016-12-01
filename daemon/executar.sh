#!/bin/bash

echo
echo

#Recupera uma pagina HTML
telnet localhost 1096
GET /buildbot-waterfall.html HTTP/1.0
echo
echo

#Recupera um aquivo BIN
telnet localhost 1096
GET /Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin HTTP/1.0
echo
echo

#Recupera um arquivo ISO
telnet localhost 1096
GET /ISOs/CentOS-6.3-x86_64-minimal.iso HTTP/1.0
echo
echo

#Recupera um arquivo PDF
telnet localhost 1096
GET /Arquivos/ProjetoNovoDPIeIPS.pdf HTTP/1.0
echo
echo
echo
echo
echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"
