#!/bin/bash
#Move todos os novos arquivos para uma nova pasta 'arquivos'
mkdir -p arquivo
mv pagina*.html arquivo/

cd arquivo/
md5sum ../../diretorio/buildbot-waterfall.html
for i in {1..100}; do
	md5sum pagina${i}.html
done


#mv imagem*.iso arquivos/
#mv pagina*.html arquivos/
#mv projeto*.pdf arquivos/
#mv binario*.bin arquivos/

#../diretorio/ISOs/CentOS-6.3-x86_64-minimal.iso 
#../buildbot-waterfall.html
#../Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
#../Arquivos/ProjetoNovoDPIeIPS.pdf


