fun main() {
    var a: Int = 3
    var b: Int = 4
    var x: Int = sum(a, b)
    for (i in 1..3) {
        println("what")
    }
    var bruh: Array<Int>(4) { 0 }
    // data[0] = 9
    // data[1] = 8
    // data[2] = data[0] + data[1]
    // println(data[2])
}


fun sum(x: Int, y: Int): Int {
    return x + y
}