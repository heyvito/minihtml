# frozen_string_literal: true

require "mkmf"

append_cflags("-fvisibility=hidden")

create_makefile("minihtml/minihtml_scanner")
