fun main() {
    var a: Int = 3
    var b: Int = 4
    var data: Array<Int>(8) {0}
    data[0] = 9
    data[1] = 8
    data[2] = data[0] + data[1]
    println(data[2])
}