-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local cyw43 = require 'pico.cyw43'
local wifi = require 'pico.cyw43.wifi'
local time = require 'pico.time'

function wifi_connect(ssid, password, auth, timeout)
    local deadline = time.make_timeout_time_ms(timeout)
    local status = cyw43.LINK_NONET
    while true do
        if status == cyw43.LINK_NONET then
            local ok, err = wifi.join(ssid, password, auth)
            if not ok then return nil, pico.error_str(err) end
        end
        status = cyw43.tcpip_link_status(cyw43.ITF_STA)
        if status == cyw43.LINK_UP then return true
        elseif status < 0 then return nil, cyw43.link_status_str(status)
        elseif time.get_absolute_time_int() - deadline > 0 then
            return nil, "connection timed out"
        end
        time.sleep_ms(200)
    end
end
