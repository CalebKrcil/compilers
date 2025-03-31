fun classify(x: Int): String {
    return when (x) {
        1 -> "one"
        2 -> "two"
        else -> "other"
    }
}
fun main() {
    println(classify(2))
}