<script lang="ts">
    import { onMount, onDestroy } from 'svelte';
    import SettingsCard from './SettingsCard.svelte';
    import { getJsonRest, postJsonRest } from '$lib/stores/rest';
    import { notifications } from "$lib/components/toasts/notifications";
    import Text from './Text.svelte';
    import Spinner from './Spinner.svelte';
    import Terminal from '~icons/tabler/terminal';
    import Select from './Select.svelte';
	import type { SpinRemote } from '$lib/types/models';
	import Checkbox from './Checkbox.svelte';
	import Slider from './Slider.svelte';
	import Collapsible from './Collapsible.svelte';

    export let restPath: string;

    let espnowState = {
        enabled: false,
        autoPing: false,
        messageHistory: [],
        retryDelay: 0,
        channel: 0
    };

    let message: string = '';
    let targetAddress: string = '';
    let remotes: SpinRemote[] = [];
    let remoteAddresses: { label: string; value: string }[] = [];

    async function getRemotes() {
        return getJsonRest('/rest/spin-seq/remotes', { remotes: [] }).then((data) => {
            remotes = data.remotes;
            console.log("ESPNow Remotes: ", remotes);
            remoteAddresses = remotes
                .filter(r => r.macAddress)
                .map(r => ({
                    label: `${r.hostname} (${r.macAddress})`,
                    value: r.macAddress
                }));
            if (!targetAddress && remoteAddresses.length > 0) {
                targetAddress = remoteAddresses[0].value;
            }
        });
    }

    async function getEspnowState() {
        return getJsonRest(restPath, espnowState).then((data) => {
            espnowState = data;
        });
    }    

    async function postEspnowState() {
        return postJsonRest(restPath, espnowState).then(() => {
            getEspnowState();
        }).catch((error) => {
            notifications.error(error.message, 500);
        });
    }

    async function sendMessage() {
        return postJsonRest(restPath + "/sendMessage", {
            message: message,
            address: targetAddress
        }).then(() => {
            message = '';
        }).catch((error) => {
            notifications.error(error.message, 500);
        });
    }

    async function getMessageHistory() {
        return getJsonRest(restPath + "/messageHistory", espnowState.messageHistory).then((data) => {
            espnowState.messageHistory = data;
        });
    }

    let intervalID: any;
    onMount(() => {
        getRemotes();
        intervalID = setInterval(() => {
            getMessageHistory();
        }, 1122);
    });
    onDestroy(() => {
        clearInterval(intervalID);
    });
</script>

<style>
    .autoscrollable-wrapper {
        overflow: auto;
        max-height: 100%;
        display: flex;
        flex-direction: column-reverse;
    }
    .autoscrollable-wrapper div {
        width: 100%;
        overflow-wrap: break-word;
        word-break: break-all;
    }
</style>

<SettingsCard>
    <Terminal slot="icon" class="flex-shrink-0 mr-2 h-6 w-6 self-end" />
    <span slot="title">ESPNow Console</span>
    {#await getEspnowState()}
        <Spinner></Spinner>
    {:then nothing}
    <div class="mt-4 bg-gray-900 text-gray-100 rounded-md p-4 h-64 overflow-y-auto font-mono text-sm autoscrollable-wrapper">
        {#each espnowState.messageHistory.reverse() as message}
            {#if message}
            <div class="py-1">
                <span class="text-gray-500">#</span>
                <span class="ml-2">{message}</span>
            </div>
            {/if}
        {/each}
    </div>
    <!-- <Checkbox
        label="Enable ESPNow"
        bind:value={espnowState.enabled}
        onChange={postEspnowState}
    /> -->
    {/await}
    {#if remoteAddresses.length > 0}
        <Select
            label="Target Device"
            bind:value={targetAddress}
        >
            <option value="FF:FF:FF:FF:FF:FF">Broadcast (FF:FF:FF:FF:FF:FF)</option>
            
            {#each remoteAddresses as remote}
                <option value={remote.value}>{remote.label}</option>
            {/each}
        </Select>
    {:else}
        <Text
            label="Target MAC Address"
            bind:value={targetAddress}
        />
    {/if}
    <Text
        label="Message"
        bind:value={message}
        onChange={sendMessage}
    />
    <Collapsible>
        <span slot="title">Settings</span>
        <Checkbox
            label="Auto Ping"
            bind:value={espnowState.autoPing}
            onChange={postEspnowState}
        />
        <Slider
            label="Broadcast Retry Delay"
            min={0}
            max={30}
            step={1}
            bind:value={espnowState.retryDelay}
            onChange={postEspnowState}
        />
        <Slider
            label="ESPNow Channel"
            min={0}
            max={14}
            step={1}
            bind:value={espnowState.channel}
            onChange={postEspnowState}
        />
    </Collapsible>
</SettingsCard>
