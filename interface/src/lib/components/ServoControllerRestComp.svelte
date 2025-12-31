<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
    import Collapsible from '$lib/components/Collapsible.svelte';
	import type { ServoControllerState } from '$lib/types/models'
	import Slider from '$lib/components/Slider.svelte';
	import Checkbox from '$lib/components/Checkbox.svelte';
	import GridForm from '$lib/components/GridForm.svelte';
	import Spinner from '$lib/components/Spinner.svelte';
	import SettingsCard from './SettingsCard.svelte';
	import { getJsonRest, postJsonRest } from '$lib/stores/rest';
	import DisableButton from './DisableButton.svelte';
	import StopButton from './StopButton.svelte';
    import Button from './Button.svelte';

    export let label: string;
    export let restPath : string;

    let controllerState : ServoControllerState = {
        position: 0,
        target: 0,
        tolerance: 0.1,
        offset: 0,
        enabled: false,
        invert: false,
        plot: false,
        encoderError: false,
        P: 10,
        I: 0,
        D: 0,
        S: 0,
        maxSpeed: 0,
        curGain: 0,
        derivative: 0,
        integral: 0,
        limits: {
            enabled: false,
            begin: 0,
            end: 360
        },
            autotune: {
                active: false,
                done: false,
                amp: 0.25,
                band: 1.0,
                Ku: 0,
                Tu: 0
            }
    };

    let intervalID: any;
    onMount(() => {
        intervalID = setInterval(()=>{
            getPosition();
        }, 987);
    });
    onDestroy(()=> {
        clearInterval(intervalID);
    });

    async function getPosition() {
        getJsonRest(restPath + "/position", controllerState.position).then((data) => controllerState.position = data);
    }

    async function getControllerState() {
        return getJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

    function postControllerState() {
        postJsonRest(restPath, controllerState).then((data) => controllerState = data);
    }

        async function startAutotune() {
            await postJsonRest(restPath, { autotune: { start: true, amp: controllerState.autotune?.amp, band: controllerState.autotune?.band }}).then((data) => controllerState = data);
        }

        async function cancelAutotune() {
            await postJsonRest(restPath, { autotune: { cancel: true }}).then((data) => controllerState = data);
        }
</script>

<SettingsCard>
    {#snippet title()}
        <span>{label} Controller</span>
    {/snippet}
    {#await getControllerState()}
    <Spinner></Spinner>
    {:then nothing}
        <div>
            <GridForm>
                <Slider 
                    label="Position" 
                    bind:value={controllerState.position}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    disabled
                ></Slider>
                <Slider 
                    label="Target" 
                    bind:value={controllerState.target}
                    min={controllerState.limits.enabled ? (controllerState.limits.begin < controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 0} 
                    max={controllerState.limits.enabled ? (controllerState.limits.begin > controllerState.limits.end ? controllerState.limits.begin : controllerState.limits.end) : 360}
                    step={0.01}
                    onChange={() => postJsonRest(restPath, {target: controllerState.target})}
                ></Slider>
                <Slider 
                    label="Speed" 
                    bind:value={controllerState.maxSpeed}
                    min={0} 
                    max={30} 
                    step={0.1}
                    onChange={() => postJsonRest(restPath, {maxSpeed: controllerState.maxSpeed})}
                ></Slider>
            </GridForm>
            <Collapsible>
                {#snippet title()}
                    <span>Settings</span>
                {/snippet}
                <span class="text-lg">Control</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Checkbox 
                        label="Invert" 
                        bind:value={controllerState.invert}
                        onChange={postControllerState}
                    ></Checkbox>
                    <Slider 
                        label="Offset" 
                        bind:value={controllerState.offset}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="Tolerance" 
                        bind:value={controllerState.tolerance}
                        min={0.04} 
                        max={0.4} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="P Gain" 
                        bind:value={controllerState.P}
                        min={0} 
                        max={20} 
                        step={0.1}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="I Gain" 
                        bind:value={controllerState.I}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="D Gain" 
                        bind:value={controllerState.D}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="S Gain" 
                        bind:value={controllerState.S}
                        min={0} 
                        max={1} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="Max Speed (deg/s)" 
                        bind:value={controllerState.maxSpeed}
                        min={0} 
                        max={30} 
                        step={0.1}
                        onChange={postControllerState}
                    ></Slider>
                        <span class="text-lg col-span-full">Autotune</span>
                        <Slider
                            label="Relay Amp"
                            bind:value={controllerState.autotune.amp}
                            min={0.05}
                            max={1}
                            step={0.01}
                            onChange={() => controllerState = { ...controllerState, autotune: { ...controllerState.autotune, amp: controllerState.autotune.amp } }}
                            disabled={controllerState.autotune.active}
                        ></Slider>
                        <Slider
                            label="Relay Band (deg)"
                            bind:value={controllerState.autotune.band}
                            min={0.1}
                            max={5}
                            step={0.1}
                            onChange={() => controllerState = { ...controllerState, autotune: { ...controllerState.autotune, band: controllerState.autotune.band } }}
                            disabled={controllerState.autotune.active}
                        ></Slider>
                        <div class="flex flex-row gap-2 items-center col-span-full">
                            <Button onClick={startAutotune} disabled={controllerState.autotune.active}>Start Autotune</Button>
                            <Button onClick={cancelAutotune} kind="ghost" disabled={!controllerState.autotune.active}>Cancel</Button>
                            {#if controllerState.autotune.active}
                                <span class="text-sm opacity-70">Running...</span>
                            {:else if controllerState.autotune.done}
                                <span class="text-sm opacity-70">Done. P={controllerState.P.toFixed(3)} I={controllerState.I.toFixed(3)} D={controllerState.D.toFixed(3)}</span>
                            {/if}
                        </div>
                        <div class="grid grid-cols-3 gap-2 text-sm col-span-full opacity-70">
                            <div>Ku: {controllerState.autotune.Ku.toFixed(3)}</div>
                            <div>Tu: {controllerState.autotune.Tu.toFixed(3)} s</div>
                            <div>Output: {controllerState.curGain.toFixed(3)}</div>
                        </div>
                    <Checkbox 
                        label="Plot" 
                        bind:value={controllerState.plot}
                        onChange={postControllerState}
                    ></Checkbox>
                </GridForm>
                <span class="text-lg">Limits</span>
                <GridForm>
                    <Checkbox 
                        label="Enable" 
                        bind:value={controllerState.limits.enabled}
                        onChange={postControllerState}
                    ></Checkbox>
                    {#if controllerState.limits.enabled}
                    <Slider 
                        label="Begin" 
                        bind:value={controllerState.limits.begin}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    <Slider 
                        label="End" 
                        bind:value={controllerState.limits.end}
                        min={0} 
                        max={360} 
                        step={0.01}
                        onChange={postControllerState}
                    ></Slider>
                    {/if}
                </GridForm>
            </Collapsible>
        </div>
    {/await}
    <div class="flex flex-row flex-wrap justify-between gap-x-2">
        <div class="flex-grow"></div>
        <div>
            <div>
                <DisableButton onClick={() => postJsonRest(restPath, {enabled: false})}></DisableButton>
                <StopButton onClick={() => postJsonRest(restPath, {enabled: false})}></StopButton>
            </div>
        </div>
    </div>
</SettingsCard>