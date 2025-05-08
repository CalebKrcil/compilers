fun main() {
    var a: Double = -4.0
    var b: Double = 2.0

    var ab: Double = java.lang.Math.abs(a)
    var mx: Double = java.lang.Math.max(a, b)
    var mn: Double = java.lang.Math.min(a, b)
    var pw: Double = java.lang.Math.pow(a, b)
    var sn: Double = java.lang.Math.sin(b)
    var cs: Double = java.lang.Math.cos(b)
    var tn: Double = java.lang.Math.tan(b)
    var r: Int = java.util.Random.nextInt()

    println("Math tests:\n")
    println("Random number: ")
    println(r)
    println("abs -4.0: ")
    println(ab)
    println("max -4.0, 2.0: ")
    println(mx)
    println("min -4.0, 2.0: ")
    println(mn)
    println("pow -4.0, 2.0: ")
    println(pw)
    println("sin 2.0: ")
    println(sn)
    println("cos 2.0: ")
    println(cs)
    println("tan 2.0: ")
    println(tn)
}
