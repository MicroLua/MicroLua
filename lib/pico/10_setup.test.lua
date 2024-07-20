-- Copyright 2024 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

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
    for i = 0, netif.IPV6_NUM_ADDRESSES - 1 do
        local ip, state = nif:ip6(i)
        if state & (ip6.ADDR_TENTATIVE | ip6.ADDR_VALID) ~= 0
                and not ip:is_linklocal() then
            return true
        end
    end
end

local function wait_non_linklocal_addr(nif, timeout)
    local dl = time.deadline(timeout)
    while time.compare(time.ticks(), dl) < 0 do
        if has_non_linklocal_addr(nif) then return true end
    end
end

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
        t:assert(wait_non_linklocal_addr(netif.default(), 10 * time.sec),
                 "no non-linklocal IPv6 address")
    end)
end
