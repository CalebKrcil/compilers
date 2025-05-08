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
    println("print test:\n")
    println("meow\n")
    println("print test with function:\n")
    silly("meow\n")
    var d: Int = multiple_args(1, 2)
    println("Integer addition of 1 and 2:\n")
    println(d)
    d++
    println("Incremented d:\n")
    println(d)
    d--
    d--
    println("Decremented d twice:\n")
    println(d)
    println("Integer addition of 1 and 2 but with function:\n")
    println(multiple_args(a, b))
    return;
}
