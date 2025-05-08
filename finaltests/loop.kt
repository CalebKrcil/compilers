fun main(){
    var i: Int = 0
    println("loop 1-5:\n")
    for (i in 1..5) {
        println(i)
    }
    var a: Array<Int> = Array<Int>(8) {0}
    println("array loop:\n")
    for (i in 0..7) {
        a[i] = i
        println(a[i])
    }
}