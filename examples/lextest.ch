import "time"

const t = time.parse("03.12.2019 11:30:32", "%d.%m.%Y %H:%M:%S")
print(t.to_local())
