func currentfiber = @"charly.builtin.core.currentfiber"()

const t = spawn {
    echo("t: {currentfiber()}")
    return 2000
}

echo("main: {currentfiber()}")

return await t