-- Copyright 2023 Remy Blank <remy@c-space.org>
-- SPDX-License-Identifier: MIT

_ENV = module(...)

local string = require 'string'

-- TODO: Document

-- Return a shell-escaped version of the argument.
function quote(s)
    if s == '' then return "''" end
    if not s:find('[^%w@%%+=:,./-]') then return s end
    return ("'%s'"):format(s:gsub("'", "'\"'\"'"))
end
