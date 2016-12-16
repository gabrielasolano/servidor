#!/bin/bash
#Move todos os novos arquivos para uma nova pasta 'arquivos'
mkdir -p arquivo
mv imagem*.iso arquivos/

md5sum ../diretorio/ISOs/CentOS-6.3-x86_64-minimal.iso
for i in {1..10}; do
	md5sum arquivos/imagem${i}.iso
done


#mv imagem*.iso arquivos/
#mv pagina*.html arquivos/
#mv projeto*.pdf arquivos/
#mv binario*.bin arquivos/

#../diretorio/ISOs/CentOS-6.3-x86_64-minimal.iso 
#../buildbot-waterfall.html
#../Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
#../Arquivos/ProjetoNovoDPIeIPS.pdf


