openocd \
  -c "gdb_port 50000" \
  -c "tcl_port 50001" \
  -c "telnet_port 50002" \
  -f tools/debug/openocd/stm32f103.cfg \
  -c "init"