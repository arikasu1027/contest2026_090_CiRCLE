if(CONFIG_LVX_USE_DEMO_CONTEST2026_090_BTPAND)
  nuttx_add_application(
    NAME btpand
    SRCS btpand_main.c
    STACKSIZE 8192)
endif()
