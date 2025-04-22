fun silly(meow: String) {
    println(meow)
    return
}

fun multiple_args(a: Int, b: Int): Int{
    var c : Int = a+b
    return c
}

fun main() {
    var a: Int = 1
    var b: Int = 2
    var c: Int = a*b
    println("meow")
    silly("meow")
    c = multiple_args(1, 2)
    println(multiple_args(a, b))
    return;
}
