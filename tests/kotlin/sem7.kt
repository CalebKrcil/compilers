object Config {
    val version = "1.0"
}
fun maybe(flag: Boolean): String? {
    return if (flag) "Yes" else null
}
fun main() {
    println(Config.version)
    println(maybe(false))
}