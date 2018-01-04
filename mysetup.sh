REL=v06_61_00
LARREL=$REL
DUNREL=${REL}
USEDEV=true
source $HOME/ups/setups
if [ -n "$USEDEV" ]; then
  PRODUCTS=/home/dladams/dudev/dudev01/workdir/localProducts_larsoft_${LARREL}_e14_prof:$PRODUCTS
fi
#unsetup dunetpc
#if [ -z "$UPS_DIR" ]; then
#fi
setup dunetpc ${DUNREL} -q e14:prof
PS1='dunefemb> '
