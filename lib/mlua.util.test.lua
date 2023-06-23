_ENV = require 'mlua.module'(...)

function test_Function_close(t)
    local called = false
    do
        local f<close> = function() called = true end
    end
    t:expect(called, "To-be-closed function wasn't called")

    local called = false
    pcall(function()
        local want = "Some error"
        local f<close> = function(err)
            called = true
            t:expect(err == want,
                     "Unexpected error: got %q, want %q", err, want)
        end
        error(want, 0)
    end)
    t:expect(called, "To-be-closed function wasn't called on error")
end
