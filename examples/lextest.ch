const response = {}
const http = {}

func then = null
func fail = null

match (response.status) {
  http.status(200)  => then(response)
  _                 => fail(response)
}
