cmd_/home/wsy/driver/of_led/modules.order := {   echo /home/wsy/driver/of_led/of_led.ko; :; } | awk '!x[$$0]++' - > /home/wsy/driver/of_led/modules.order
