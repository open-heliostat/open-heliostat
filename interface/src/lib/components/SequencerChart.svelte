<script lang="ts">
    import { onMount, onDestroy, afterUpdate, tick } from 'svelte';
    import { createEventDispatcher } from 'svelte';
    import type { SequencerKeyframe } from '$lib/types/models';
    import type { Chart as ChartType, ActiveElement } from 'chart.js/auto';

    export let label = 'Sequence';
    export let frames: SequencerKeyframe[] = [];
    export let running = false;
    export let loop = false;

    const dispatch = createEventDispatcher<{ change: { frames: SequencerKeyframe[] } }>();

    let ChartCtor: typeof import('chart.js/auto').default | null = null;
    let chart: ChartType;
    let chartEl: HTMLCanvasElement;
    let dragging = false;
    let dragIdx: number | null = null;
    let lockedExtents: { xMin: number; xMax: number; yMin: number; yMax: number } | null = null;
    let playheadMs = 0;
    let durationMs = 0;
    let rafId: number | null = null;
    let lastFrameTs = 0;
    let wasRunning = false;
    let workingFrames: SequencerKeyframe[] = [];
    $: workingFrames = frames;

    onMount(async () => {
        await tick();
        const mod = await import('chart.js/auto');
        ChartCtor = mod.default;
        await tick();
        await initWhenReady();
    });

    afterUpdate(() => {
        if (!chart && chartEl && ChartCtor) {
            buildChart();
            syncChart();
        }
    });

    onDestroy(() => {
        if (chart) chart.destroy();
        stopPlayheadLoop();
    });

    function sortFrames(arr: SequencerKeyframe[]) {
        return arr.slice().sort((a, b) => a.tMs - b.tMs);
    }

    async function initWhenReady() {
        for (let i = 0; i < 10; i++) {
            if (chartEl) break;
            await tick();
            await new Promise((resolve) => setTimeout(resolve, 50));
        }
        if (!chartEl) return;
        buildChart();
        syncChart();
    }

    function buildChart() {
        if (chart) chart.destroy();
        if (!chartEl || !ChartCtor) return;
        const ctx = chartEl.getContext('2d');
        if (!ctx) return;

        // Ensure the canvas has dimensions even if flex parents haven't sized yet
        const cw = chartEl.clientWidth;
        const ch = chartEl.clientHeight;
        chartEl.width = cw > 0 ? cw : 800;
        chartEl.height = ch > 0 ? ch : 360;

        const playheadPlugin = {
            id: 'playheadLine',
            afterDatasetsDraw(chartInstance: ChartType) {
                if (!durationMs) return;
                const xScale = chartInstance.scales.x;
                const { top, bottom } = chartInstance.chartArea;
                const x = xScale.getPixelForValue(playheadMs);
                if (Number.isNaN(x)) return;
                const ctxp = chartInstance.ctx;
                ctxp.save();
                ctxp.strokeStyle = '#f97316';
                ctxp.lineWidth = 2;
                ctxp.setLineDash([4, 3]);
                ctxp.beginPath();
                ctxp.moveTo(x, top);
                ctxp.lineTo(x, bottom);
                ctxp.stroke();
                ctxp.restore();
            }
        };

        chart = new ChartCtor(ctx, {
            type: 'line',
            data: {
                datasets: [
                    {
                        label: label + ' sequence',
                        borderColor: '#38bdf8',
                        borderWidth: 3,
                        backgroundColor: 'rgba(56,189,248,0.15)',
                        pointBorderColor: '#0ea5e9',
                        pointBackgroundColor: '#0ea5e9',
                        pointRadius: 6,
                        showLine: true,
                        spanGaps: true,
                        fill: false,
                        data: []
                    }
                ]
            },
            options: {
                animation: false,
                parsing: { xAxisKey: 'x', yAxisKey: 'y' },
                responsive: true,
                maintainAspectRatio: false,
                devicePixelRatio: window.devicePixelRatio || 1,
                scales: {
                    x: { type: 'linear', title: { display: true, text: 'Time (ms)' } },
                    y: { type: 'linear', title: { display: true, text: 'Value' } }
                },
                plugins: {
                    legend: { display: true },
                    tooltip: { enabled: true }
                }
            },
            plugins: [playheadPlugin]
        });
        chart.resize();
        syncChart();
    }

    function syncChart(nextFrames?: SequencerKeyframe[]) {
        if (!chart) return;
        const framesToUse = sortFrames(nextFrames ?? workingFrames);
        const pts = framesToUse.map((f) => ({ x: Number(f.tMs), y: Number(f.value) }));
        chart.data.datasets[0].data = pts;
        durationMs = pts.length ? Math.max(...pts.map((p) => p.x)) : 0;
        if (playheadMs > durationMs) playheadMs = durationMs;
        if (running && !wasRunning) {
            playheadMs = 0;
            lastFrameTs = 0;
        }
        wasRunning = running;

        const data = chart.data.datasets[0].data as { x: number; y: number }[];
        const xs = data.length ? data.map((p) => p.x) : [0];
        const ys = data.length ? data.map((p) => p.y) : [0];
        const xMin = Math.min(...xs);
        const xMax = Math.max(...xs);
        const yMin = Math.min(...ys);
        const yMax = Math.max(...ys);
        const padY = (yMax - yMin) * 0.1 + 1;
        const padX = (xMax - xMin) * 0.05 + 1;
        const use = lockedExtents ?? {
            xMin: xMin - padX,
            xMax: xMax + padX,
            yMin: yMin - padY,
            yMax: yMax + padY,
        };
        chart.options.scales = {
            x: { type: 'linear', min: use.xMin, max: use.xMax, title: { display: true, text: 'Time (ms)' } },
            y: { type: 'linear', min: use.yMin, max: use.yMax, title: { display: true, text: 'Value' } },
        };
        chart.update();
        scheduleResize();
        ensurePlayheadLoop();
    }

    // re-sync whenever incoming frames change
    $: if (chart) syncChart(workingFrames);

    function scheduleResize() {
        if (!chart) return;
        requestAnimationFrame(() => chart.resize());
    }

    function ensurePlayheadLoop() {
        if (!chart || !running || durationMs <= 0) {
            stopPlayheadLoop();
            return;
        }
        if (rafId !== null) return;
        lastFrameTs = 0;
        rafId = requestAnimationFrame(stepPlayhead);
    }

    function stopPlayheadLoop() {
        if (rafId !== null) {
            cancelAnimationFrame(rafId);
            rafId = null;
        }
        lastFrameTs = 0;
    }

    function stepPlayhead(ts: number) {
        if (!running || durationMs <= 0) {
            stopPlayheadLoop();
            return;
        }
        if (lastFrameTs === 0) lastFrameTs = ts;
        const dt = ts - lastFrameTs;
        lastFrameTs = ts;
        playheadMs += dt;
        if (loop) {
            playheadMs = durationMs ? playheadMs % durationMs : 0;
        } else {
            playheadMs = Math.min(playheadMs, durationMs);
        }
        chart?.update('none');
        rafId = requestAnimationFrame(stepPlayhead);
    }

    function getValueFromMouse(evt: MouseEvent) {
        if (!chart) return { xVal: NaN, yVal: NaN };
        const rect = chart.canvas.getBoundingClientRect();
        const xPixel = evt.clientX - rect.left;
        const yPixel = evt.clientY - rect.top;
        return {
            xVal: Number(chart.scales.x.getValueForPixel(xPixel)),
            yVal: Number(chart.scales.y.getValueForPixel(yPixel)),
        };
    }

    function findNearestPoint(evt: MouseEvent): number | null {
        if (!chart) return null;
        const elements = chart.getElementsAtEventForMode(evt, 'nearest', { intersect: true }, false) as ActiveElement[];
        if (elements.length) return elements[0].index;
        return null;
    }

    function commitFrames(nextFrames: SequencerKeyframe[]) {
        const sorted = sortFrames(nextFrames);
        workingFrames = sorted;
        syncChart(sorted);
        dispatch('change', { frames: sorted });
    }

    function onCanvasMouseDown(event: MouseEvent) {
        if (!chart) return;
        // right click: delete nearest point
        if (event.button === 2) {
            event.preventDefault();
            const idx = findNearestPoint(event);
            if (idx !== null) {
                const next = workingFrames.filter((_, i) => i !== idx);
                lockedExtents = null;
                commitFrames(next);
            }
            return;
        }

        const idx = findNearestPoint(event);
        if (idx !== null) {
            dragging = true;
            dragIdx = idx;
            const data = chart.data.datasets[0].data as { x: number; y: number }[];
            const xs = data.map((p) => p.x);
            const ys = data.map((p) => p.y);
            const xMin = Math.min(...xs);
            const xMax = Math.max(...xs);
            const yMin = Math.min(...ys);
            const yMax = Math.max(...ys);
            const padY = (yMax - yMin) * 0.1 + 1;
            const padX = (xMax - xMin) * 0.05 + 1;
            lockedExtents = {
                xMin: xMin - padX,
                xMax: xMax + padX,
                yMin: yMin - padY,
                yMax: yMax + padY,
            };
        } else {
            const { xVal, yVal } = getValueFromMouse(event);
            if (!Number.isNaN(xVal) && !Number.isNaN(yVal)) {
                const newFrame = { tMs: Math.max(0, Math.round(xVal)), value: Number(yVal.toFixed(2)) };
                commitFrames([...workingFrames, newFrame]);
            }
        }
    }

    function onCanvasMouseMove(event: MouseEvent) {
        if (!dragging || dragIdx === null || !chart) return;
        const { xVal, yVal } = getValueFromMouse(event);
        if (Number.isNaN(xVal) || Number.isNaN(yVal)) return;
        const next = workingFrames.slice();
        if (dragIdx >= next.length) return;
        next[dragIdx] = { tMs: Math.max(0, Math.round(xVal)), value: Number(yVal.toFixed(2)) };
        commitFrames(next);
    }

    function onCanvasMouseUp() {
        dragging = false;
        dragIdx = null;
        lockedExtents = null;
    }
</script>

<div class="w-full h-72">
    <canvas
        bind:this={chartEl}
        style="width: 100%; height: 100%; background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05);"
        on:mousedown={onCanvasMouseDown}
        on:mousemove={onCanvasMouseMove}
        on:mouseup={onCanvasMouseUp}
        on:mouseleave={onCanvasMouseUp}
        on:contextmenu|preventDefault
    ></canvas>
</div>
