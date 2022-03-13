func add(a, b) {
    a + b
}

const t = spawn {
    const result = add(2, 3)
    echo(result)
    result
}

const result = await t

echo(result)