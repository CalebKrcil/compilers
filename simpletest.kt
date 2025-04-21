fun silly(meow: String) {
    println(meow)
    return
}

fun multiple_args(a: Int, b: Int, c: Int): Int {
    return a + b + c
}

fun main() {
    var a: Int = 1
    var b: Int = 2
    var c: Int = a*b
    println("meow")
    silly("meow")
    multiple_args(1, 2, 3)
    multiple_args(a, b, c)
    return;
}
