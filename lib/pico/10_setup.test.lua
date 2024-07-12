-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local config = require 'mlua.config'
local time = require 'mlua.time'
local cyw43 = require 'pico.cyw43'
local util = require 'pico.cyw43.util'
local wifi = require 'pico.cyw43.wifi'
local lwip = require 'pico.lwip'

-- TOOD: Abort all tests if initialization fails

local module_name = ...

function set_up(t)
    t:printf("SSID: %s, PASSWORD: %s, SERVER: %s:%s\n", config.WIFI_SSID,
             config.WIFI_PASSWORD, config.SERVER_ADDR, config.SERVER_PORT)
    t:once(module_name .. '|cyw43', function()
        t:assert(cyw43.init(), "Failed to initialize CYW43")
    end)
    t:once(module_name .. '|lwip', function()
        t:assert(lwip.init(), "Failed to initialize lwIP")
    end)
    t:once(module_name .. '|wifi', function()
        wifi.set_up(cyw43.ITF_STA, true, cyw43.COUNTRY_WORLDWIDE)
        local ok, err = util.wifi_connect(
            config.WIFI_SSID, config.WIFI_PASSWORD, cyw43.AUTH_WPA2_AES_PSK,
            30 * time.sec)
        t:assert(ok, "failed to connect: %s", err)
    end)
end
