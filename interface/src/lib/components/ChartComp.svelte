<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { Chart, registerables } from 'chart.js';
	import { daisyColor } from '$lib/DaisyUiHelper';
	import { cubicOut } from 'svelte/easing';
	import { slide } from 'svelte/transition';

    export let label: string;
    export let data : number[] | undefined;
    $: { data; updateData(); }

	Chart.register(...registerables);

    let chartElement: HTMLCanvasElement;
    let chart: Chart;

    onMount(() => {
        chart = new Chart(chartElement, {
            type: 'line',
            data: {
				labels: [],
                datasets: [
                    {
                        label: label + ' calibration data',
                        borderColor: daisyColor('--p'),
                        backgroundColor: daisyColor('--p', 50),
                        borderWidth: 2,
                        data: [],
                        yAxisID: 'y'
                    }
                ]
            },
            options: {
                maintainAspectRatio: false,
                responsive: true,
                elements: {
                    point: {
                        radius: 1
                    }
                },
                scales: {
                    x: {
                        grid: {
                            color: daisyColor('--bc', 10)
                        },
                        ticks: {
                            color: daisyColor('--bc')
                        },
                        display: false
                    },
                    y: {
                        type: 'linear',
                        title: {
                            display: true,
                            text: 'Offset',
                            color: daisyColor('--bc'),
                            font: {
                                size: 16,
                                weight: 'bold'
                            }
                        },
                        position: 'left',
                        grid: { color: daisyColor('--bc', 10) },
                        ticks: {
                            color: daisyColor('--bc')
                        },
                        border: { color: daisyColor('--bc', 10) }
                    }
                }
            }
        });
        // setInterval(() => {
        //     updateData();
        // }, 2000);
    });

    function updateData() {
        if (data?.length) {
            chart.data.labels = data;
            chart.data.datasets[0].data = data;
            chart.scales['y'].min = Math.min(...data);
            chart.scales['y'].max = Math.max(...data);
            chart.update('none');
        }
    }

</script>

<div class="w-full overflow-x-auto">
    <div
        class="flex w-full flex-col space-y-1 h-52"
        transition:slide|local={{ duration: 300, easing: cubicOut }}
    >
        <canvas bind:this={chartElement} />
    </div>
</div>