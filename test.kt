fun mathstuff(num1: Int, num2: Int, num3: Int, num4: Int): Int {
    num1 = num2 + num3
    return num1 + num2 + num3 + num4
}

fun main() {
    println("test")
    println(mathstuff(1, 2, 3, 4))
}