-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

local lwip = require 'lwip'
local ip6 = require 'lwip.ip6'
local netif = require 'lwip.netif'
local config = require 'mlua.config'
local time = require 'mlua.time'
local cyw43 = require 'pico.cyw43'
local util = require 'pico.cyw43.util'
local wifi = require 'pico.cyw43.wifi'

-- TOOD: Abort all tests if initialization fails

local module_name = ...

local function has_non_linklocal_addr(nif)
    for _, ip, state in nif:ip6() do
        if state & (ip6.ADDR_TENTATIVE | ip6.ADDR_VALID) ~= 0
                and not ip:is_linklocal() then
            return true
        end
    end
end

local function wait_non_linklocal_addr(nif, deadline)
    while time.compare(time.ticks(), deadline) < 0 do
        if has_non_linklocal_addr(nif) then return true end
        time.sleep_for(100 * time.msec)
    end
end

function set_up(t)
    t:printf("cyw43: %s.%s.%s\n", cyw43.VERSION >> 16,
             (cyw43.VERSION >> 8) & 0xff, cyw43.VERSION & 0xff)
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
        local dl = time.deadline(30 * time.sec)
        local ok, err = util.wifi_connect(
            config.WIFI_SSID, config.WIFI_PASSWORD, cyw43.AUTH_WPA2_AES_PSK, dl)
        t:assert(ok, "failed to connect: %s", err)
        t:assert(wait_non_linklocal_addr(netif.default(), dl),
                 "no non-linklocal IPv6 address")
    end)
end
