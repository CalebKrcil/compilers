fun use_double(a: Double, b: Double): Double{
    var c: Double = a + b
    return c
}

fun use_ints(a: Int, b: Int): Int{
    var c: Int = a + b
    return c
}

fun main(){
    var a: Double = 5.4
    var b: Double = 2.1
    var c: Double = use_double(a, b)
    println(c)
    println(use_double(a, b))
    var d: Int = use_ints(1, 2)
    println(d)
    println(use_ints(2, 30))
    return
}