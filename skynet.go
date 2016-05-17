package main

import (
	"flag"
	"fmt"
	"time"
)

var gsize = flag.Int("s", 100000, "arg 'size' for benchmark")
var gdiv = flag.Int("d", 10, "arg 'div' for benchmark")
var titer uint64 = 0
var totalit chan int

func itercounter() {
	for {
		select {
		case a:=<-totalit:
			if a == 0 {
				return
			}
			titer++
		default:
			continue
		}
	}
}

func skynet(c chan int, num int, size int, div int) {
	if size == 1 {
		c <- num
		return
	}

	rc := make(chan int)
	var sum int
	for i := 0; i < div; i++ {
		subNum := num + i*(size/div)
		totalit <- 1
		go skynet(rc, subNum, size/div, div)
	}
	for i := 0; i < div; i++ {
		sum += <-rc
	}
	c <- sum
}

func main() {
	flag.Parse()
	c := make(chan int)
	totalit = make(chan int)
	go itercounter()
	start := time.Now()
	go skynet(c, 0, *gsize, *gdiv)
	result := <-c
	totalit <- 0
	took := time.Since(start)
	fmt.Printf("Result: %d in %d ms.\n", result, took.Nanoseconds()/1e6)
	fmt.Printf("Total Iterations: %v\n", titer)
}
