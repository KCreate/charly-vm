//Charly.set_default_exception_handler(->(e) {
  //print("Caught an exception: " + e)
//})

//try {
  //defer(->{
    //throw "hello world"
  //})
//} catch(e) {
  //print(e)
//}

class Server {
  func constructor {
    Server.instance_created = Charly.time.now_highres()
  }

  static func get_instance {
    if @instance return @instance
    @instance = Server()
  }

  static property instance = null
  static property time_start = @time.now_highres()
  static property instance_created = null
}

print(Server)

const s = Server.get_instance()

print(Server)

s.test = 100

print(s)

const s2 = Server.get_instance()

Server.instance.test = 200

print(Server)
print(s)
